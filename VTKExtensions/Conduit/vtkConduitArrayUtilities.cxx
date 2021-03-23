/*=========================================================================

  Program:   ParaView
  Module:    vtkConduitArrayUtilities.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkConduitArrayUtilities.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkSOADataArrayTemplate.h"
#include "vtkTypeFloat32Array.h"
#include "vtkTypeFloat64Array.h"
#include "vtkTypeInt16Array.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeInt64Array.h"
#include "vtkTypeInt8Array.h"
#include "vtkTypeUInt16Array.h"
#include "vtkTypeUInt32Array.h"
#include "vtkTypeUInt64Array.h"
#include "vtkTypeUInt8Array.h"

#include <conduit.hpp>
#include <conduit_blueprint_mcarray.hpp>
#include <conduit_cpp_to_c.hpp>

#include <vector>

namespace internals
{

using AOSArrays = vtkTypeList::Unique<
  vtkTypeList::Create<vtkAOSDataArrayTemplate<vtkTypeInt8>, vtkAOSDataArrayTemplate<vtkTypeInt16>,
    vtkAOSDataArrayTemplate<vtkTypeInt32>, vtkAOSDataArrayTemplate<vtkTypeInt64>,
    vtkAOSDataArrayTemplate<vtkTypeUInt8>, vtkAOSDataArrayTemplate<vtkTypeUInt16>,
    vtkAOSDataArrayTemplate<vtkTypeUInt32>, vtkAOSDataArrayTemplate<vtkTypeUInt64>,
    vtkAOSDataArrayTemplate<vtkTypeFloat32>, vtkAOSDataArrayTemplate<vtkTypeFloat64> > >::Result;

using SOAArrays = vtkTypeList::Unique<
  vtkTypeList::Create<vtkSOADataArrayTemplate<vtkTypeInt8>, vtkSOADataArrayTemplate<vtkTypeInt16>,
    vtkSOADataArrayTemplate<vtkTypeInt32>, vtkSOADataArrayTemplate<vtkTypeInt64>,
    vtkSOADataArrayTemplate<vtkTypeUInt8>, vtkSOADataArrayTemplate<vtkTypeUInt16>,
    vtkSOADataArrayTemplate<vtkTypeUInt32>, vtkSOADataArrayTemplate<vtkTypeUInt64>,
    vtkSOADataArrayTemplate<vtkTypeFloat32>, vtkSOADataArrayTemplate<vtkTypeFloat64> > >::Result;

bool is_contiguous(const conduit::Node& node)
{
  if (node.is_contiguous())
  {
    return true;
  }
  auto iter = node.children();
  while (iter.has_next())
  {
    if (!iter.next().is_contiguous())
    {
      return false;
    }
  }
  return true;
}

template <typename ArrayT>
vtkSmartPointer<ArrayT> CreateAOSArray(
  vtkIdType number_of_tuples, int number_of_components, const typename ArrayT::ValueType* raw_ptr)
{
  auto array = vtkSmartPointer<ArrayT>::New();
  array->SetNumberOfComponents(number_of_components);
  array->SetArray(const_cast<typename ArrayT::ValueType*>(raw_ptr),
    number_of_tuples * number_of_components, /*save=*/1);
  return array;
}

template <typename ValueT>
vtkSmartPointer<vtkSOADataArrayTemplate<ValueT> > CreateSOArray(
  vtkIdType number_of_tuples, int number_of_components, const std::vector<void*>& raw_ptrs)
{
  auto array = vtkSmartPointer<vtkSOADataArrayTemplate<ValueT> >::New();
  array->SetNumberOfComponents(number_of_components);
  for (int cc = 0; cc < number_of_components; ++cc)
  {
    array->SetArray(cc, reinterpret_cast<ValueT*>(raw_ptrs.at(cc)), number_of_tuples,
      /*updateMaxId=*/true, /*save*/ true);
  }
  return array;
}

//----------------------------------------------------------------------------
// internal: change components helper.
struct ChangeComponentsAOSImpl
{
  vtkDataArray* Input;
  template <typename ArrayT>
  void operator()(ArrayT* output)
  {
    using ValueType = typename ArrayT::ValueType;
    ArrayT* input = vtkArrayDownCast<ArrayT>(this->Input);
    const int numComps = std::max(input->GetNumberOfComponents(), output->GetNumberOfComponents());
    ValueType* tuple = new ValueType[numComps];
    std::fill(tuple, tuple + numComps, static_cast<ValueType>(0));
    for (vtkIdType cc = 0, max = input->GetNumberOfTuples(); cc < max; ++cc)
    {
      input->GetTypedTuple(cc, tuple);
      output->SetTypedTuple(cc, tuple);
    }
    delete[] tuple;
  }
};

//----------------------------------------------------------------------------
// internal: change components.
static vtkSmartPointer<vtkDataArray> ChangeComponentsAOS(vtkDataArray* array, int num_components)
{
  vtkSmartPointer<vtkDataArray> result;
  result.TakeReference(array->NewInstance());
  result->SetName(array->GetName());
  result->SetNumberOfComponents(num_components);
  result->SetNumberOfTuples(array->GetNumberOfTuples());

  ChangeComponentsAOSImpl worker{ array };
  using Dispatch = vtkArrayDispatch::DispatchByArray<AOSArrays>;
  if (!Dispatch::Execute(result, worker))
  {
    std::runtime_error("Failed to strip extra components from array!");
  }
  return result;
}

struct ChangeComponentsSOAImpl
{
  int Target;

  template <typename ValueT>
  void operator()(vtkSOADataArrayTemplate<ValueT>* array)
  {
    const auto numTuples = array->GetNumberOfTuples();
    const auto numComps = array->GetNumberOfComponents();
    array->SetNumberOfComponents(this->Target);

    ValueT* buffer = new ValueT[numTuples];
    std::fill_n(buffer, numTuples, 0);

    for (int cc = numComps; cc < this->Target; ++cc)
    {
      array->SetArray(cc, buffer, numTuples, /*updateMaxId=*/true,
        /*save=*/cc == numComps, /*deletMethod*/ vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
    }
  }
};
//----------------------------------------------------------------------------
static vtkSmartPointer<vtkDataArray> ChangeComponentsSOA(vtkDataArray* array, int num_components)
{
  if (array->GetNumberOfComponents() > num_components)
  {
    array->SetNumberOfComponents(num_components);
    return array;
  }

  ChangeComponentsSOAImpl worker{ num_components };
  using Dispatch = vtkArrayDispatch::DispatchByArray<SOAArrays>;
  if (!Dispatch::Execute(array, worker))
  {
    std::runtime_error("Failed to strip extra components from array!");
  }
  return array;
}

//----------------------------------------------------------------------------
conduit::index_t GetTypeId(conduit::index_t type, bool force_signed)
{
  if (!force_signed)
  {
    return type;
  }
  switch (type)
  {
    case conduit::DataType::UINT8_ID:
      return conduit::DataType::INT8_ID;

    case conduit::DataType::UINT16_ID:
      return conduit::DataType::INT16_ID;

    case conduit::DataType::UINT32_ID:
      return conduit::DataType::INT32_ID;

    case conduit::DataType::UINT64_ID:
      return conduit::DataType::INT64_ID;

    default:
      return type;
  }
}

} // internals

vtkStandardNewMacro(vtkConduitArrayUtilities);
//----------------------------------------------------------------------------
vtkConduitArrayUtilities::vtkConduitArrayUtilities() = default;

//----------------------------------------------------------------------------
vtkConduitArrayUtilities::~vtkConduitArrayUtilities() = default;

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkConduitArrayUtilities::MCArrayToVTKArray(
  const conduit_node* mcarray, const std::string& arrayname)
{
  if (auto array = vtkConduitArrayUtilities::MCArrayToVTKArray(mcarray))
  {
    array->SetName(arrayname.c_str());
    return array;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkConduitArrayUtilities::MCArrayToVTKArray(
  const conduit_node* mcarray)
{
  return vtkConduitArrayUtilities::MCArrayToVTKArrayImpl(mcarray, false);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkConduitArrayUtilities::MCArrayToVTKArrayImpl(
  const conduit_node* c_mcarray, bool force_signed)
{
  const conduit::Node& mcarray = (*conduit::cpp_node(c_mcarray));

  conduit::Node info;
  if (!conduit::blueprint::mcarray::verify(mcarray, info))
  {
    // in some-cases, this may directly be an array of numeric values; is so, handle that.
    if (mcarray.dtype().is_number())
    {
      conduit::Node temp;
      temp.append().set_external(mcarray);
      return vtkConduitArrayUtilities::MCArrayToVTKArrayImpl(conduit::c_node(&temp), force_signed);
    }
    else
    {
      vtkLogF(ERROR, "invalid node of type '%s'", mcarray.dtype().name().c_str());
      return nullptr;
    }
  }

  const int number_of_components = mcarray.number_of_children();
  if (number_of_components <= 0)
  {
    vtkLogF(ERROR, "invalid number of components '%d'", number_of_components);
    return nullptr;
  }

  // confirm that all components have same type. we don't support mixed component types currently.
  // we can easily by deep copying, but we won't until needed.

  for (conduit::index_t cc = 1; cc < mcarray.number_of_children(); ++cc)
  {
    auto& dtype0 = mcarray.child(0).dtype();
    auto& dtypeCC = mcarray.child(cc).dtype();
    if (dtype0.id() != dtypeCC.id())
    {
      vtkLogF(ERROR,
        "mismatched component types for component 0 (%s) and %d (%s); currently not supported.",
        dtype0.name().c_str(), static_cast<int>(cc), dtypeCC.name().c_str());
      return nullptr;
    }
  }

  if (conduit::blueprint::mcarray::is_interleaved(mcarray))
  {
    return vtkConduitArrayUtilities::MCArrayToVTKAOSArray(conduit::c_node(&mcarray), force_signed);
  }
  else if (internals::is_contiguous(mcarray))
  {
    return vtkConduitArrayUtilities::MCArrayToVTKSOAArray(conduit::c_node(&mcarray), force_signed);
  }
  else
  {
    // TODO: we can do a deep-copy in this case, so we can still handle it quite easily when needed.
    vtkLogF(ERROR, "unsupported array layout.");
    return nullptr;
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkConduitArrayUtilities::MCArrayToVTKAOSArray(
  const conduit_node* c_mcarray, bool force_signed)
{
  const conduit::Node& mcarray = (*conduit::cpp_node(c_mcarray));
  auto& child0 = mcarray.child(0);
  auto& dtype0 = child0.dtype();

  const int num_components = static_cast<int>(mcarray.number_of_children());
  const vtkIdType num_tuples = static_cast<vtkIdType>(dtype0.number_of_elements());

  switch (internals::GetTypeId(dtype0.id(), force_signed))
  {
    case conduit::DataType::INT8_ID:
      return internals::CreateAOSArray<vtkTypeInt8Array>(num_tuples, num_components,
        reinterpret_cast<const vtkTypeInt8Array::ValueType*>(child0.element_ptr(0)));

    case conduit::DataType::INT16_ID:
      return internals::CreateAOSArray<vtkTypeInt16Array>(num_tuples, num_components,
        reinterpret_cast<const vtkTypeInt16Array::ValueType*>(child0.element_ptr(0)));

    case conduit::DataType::INT32_ID:
      return internals::CreateAOSArray<vtkTypeInt32Array>(num_tuples, num_components,
        reinterpret_cast<const vtkTypeInt32Array::ValueType*>(child0.element_ptr(0)));

    case conduit::DataType::INT64_ID:
      return internals::CreateAOSArray<vtkTypeInt64Array>(num_tuples, num_components,
        reinterpret_cast<const vtkTypeInt64Array::ValueType*>(child0.element_ptr(0)));

    case conduit::DataType::UINT8_ID:
      return internals::CreateAOSArray<vtkTypeUInt8Array>(num_tuples, num_components,
        reinterpret_cast<const vtkTypeUInt8Array::ValueType*>(child0.element_ptr(0)));

    case conduit::DataType::UINT16_ID:
      return internals::CreateAOSArray<vtkTypeUInt16Array>(num_tuples, num_components,
        reinterpret_cast<const vtkTypeUInt16Array::ValueType*>(child0.element_ptr(0)));

    case conduit::DataType::UINT32_ID:
      return internals::CreateAOSArray<vtkTypeUInt32Array>(num_tuples, num_components,
        reinterpret_cast<const vtkTypeUInt32Array::ValueType*>(child0.element_ptr(0)));

    case conduit::DataType::UINT64_ID:
      return internals::CreateAOSArray<vtkTypeUInt64Array>(num_tuples, num_components,
        reinterpret_cast<const vtkTypeUInt64Array::ValueType*>(child0.element_ptr(0)));

    case conduit::DataType::FLOAT32_ID:
      return internals::CreateAOSArray<vtkTypeFloat32Array>(num_tuples, num_components,
        reinterpret_cast<const vtkTypeFloat32Array::ValueType*>(child0.element_ptr(0)));

    case conduit::DataType::FLOAT64_ID:
      return internals::CreateAOSArray<vtkTypeFloat64Array>(num_tuples, num_components,
        reinterpret_cast<const vtkTypeFloat64Array::ValueType*>(child0.element_ptr(0)));

    default:
      vtkLogF(ERROR, "unsupported data type '%s' ", dtype0.name().c_str());
      return nullptr;
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkConduitArrayUtilities::MCArrayToVTKSOAArray(
  const conduit_node* c_mcarray, bool force_signed)
{
  const conduit::Node& mcarray = (*conduit::cpp_node(c_mcarray));
  auto& dtype0 = mcarray.child(0).dtype();
  const int num_components = static_cast<int>(mcarray.number_of_children());
  const vtkIdType num_tuples = static_cast<vtkIdType>(dtype0.number_of_elements());

  std::vector<void*> ptrs;
  for (int cc = 0; cc < num_components; ++cc)
  {
    ptrs.push_back(const_cast<void*>(mcarray.child(cc).element_ptr(0)));
  }

  switch (internals::GetTypeId(dtype0.id(), force_signed))
  {
    case conduit::DataType::INT8_ID:
      return internals::CreateSOArray<vtkTypeInt8>(num_tuples, num_components, ptrs);

    case conduit::DataType::INT16_ID:
      return internals::CreateSOArray<vtkTypeInt16>(num_tuples, num_components, ptrs);

    case conduit::DataType::INT32_ID:
      return internals::CreateSOArray<vtkTypeInt32>(num_tuples, num_components, ptrs);

    case conduit::DataType::INT64_ID:
      return internals::CreateSOArray<vtkTypeInt64>(num_tuples, num_components, ptrs);

    case conduit::DataType::UINT8_ID:
      return internals::CreateSOArray<vtkTypeUInt8>(num_tuples, num_components, ptrs);

    case conduit::DataType::UINT16_ID:
      return internals::CreateSOArray<vtkTypeUInt16>(num_tuples, num_components, ptrs);

    case conduit::DataType::UINT32_ID:
      return internals::CreateSOArray<vtkTypeUInt32>(num_tuples, num_components, ptrs);

    case conduit::DataType::UINT64_ID:
      return internals::CreateSOArray<vtkTypeUInt64>(num_tuples, num_components, ptrs);

    case conduit::DataType::FLOAT32_ID:
      return internals::CreateSOArray<vtkTypeFloat32>(num_tuples, num_components, ptrs);

    case conduit::DataType::FLOAT64_ID:
      return internals::CreateSOArray<vtkTypeFloat64>(num_tuples, num_components, ptrs);

    default:
      vtkLogF(ERROR, "unsupported data type '%s' ", dtype0.name().c_str());
      return nullptr;
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkConduitArrayUtilities::SetNumberOfComponents(
  vtkDataArray* array, int num_components)
{
  if (array == nullptr || array->GetNumberOfComponents() == num_components)
  {
    return array;
  }

  if (array->HasStandardMemoryLayout())
  {
    return internals::ChangeComponentsAOS(array, num_components);
  }
  else
  {
    return internals::ChangeComponentsSOA(array, num_components);
  }
}

struct NoOp
{
  template <typename T>
  void operator()(T*)
  {
  }
};

//----------------------------------------------------------------------------
vtkSmartPointer<vtkCellArray> vtkConduitArrayUtilities::MCArrayToVTKCellArray(
  vtkIdType cellSize, const conduit_node* mcarray)
{
  auto array = vtkConduitArrayUtilities::MCArrayToVTKArrayImpl(mcarray, /*force_signed*/ true);
  if (!array)
  {
    return nullptr;
  }

  // now the array matches the type accepted by vtkCellArray (in most cases).
  vtkNew<vtkCellArray> cellArray;
  cellArray->SetData(cellSize, array);
  return cellArray;
}

//----------------------------------------------------------------------------
void vtkConduitArrayUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
