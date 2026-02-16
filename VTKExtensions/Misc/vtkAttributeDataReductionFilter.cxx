// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAttributeDataReductionFilter.h"

#include "vtkArrayDispatch.h"
#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include <cassert>
#include <vector>

vtkStandardNewMacro(vtkAttributeDataReductionFilter);
//-----------------------------------------------------------------------------
vtkAttributeDataReductionFilter::vtkAttributeDataReductionFilter()
{
  this->ReductionType = vtkAttributeDataReductionFilter::ADD;
  this->AttributeType = vtkAttributeDataReductionFilter::POINT_DATA |
    vtkAttributeDataReductionFilter::CELL_DATA | vtkAttributeDataReductionFilter::ROW_DATA;
}

//-----------------------------------------------------------------------------
vtkAttributeDataReductionFilter::~vtkAttributeDataReductionFilter() = default;

//----------------------------------------------------------------------------
int vtkAttributeDataReductionFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return this->Superclass::FillInputPortInformation(port, info);
}

//----------------------------------------------------------------------------
int vtkAttributeDataReductionFilter::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  vtkDataObject* input = vtkDataObject::GetData(inInfo);
  if (input)
  {
    this->GetOutputPortInformation(0)->Set(
      vtkDataObject::DATA_EXTENT_TYPE(), input->GetExtentType());

    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkDataSet* output = vtkDataSet::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));

    if (!output || !output->IsA(input->GetClassName()))
    {
      vtkDataObject* newOutput = input->NewInstance();
      info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
      newOutput->Delete();
    }
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
namespace
{
struct vtkAttributeDataReductionFilterReduceWorker
{
  template <class TArrayIn, class TArrayOut, class TIn = vtk::GetAPIType<TArrayIn>,
    class TOut = vtk::GetAPIType<TArrayOut>>
  void operator()(TArrayIn* inArray, TArrayOut* outArray, vtkAttributeDataReductionFilter* self,
    double progress_offset, double progress_factor)
  {
    auto in = vtk::DataArrayValueRange(inArray);
    auto out = vtk::DataArrayValueRange(outArray);
    int mode = self->GetReductionType();
    vtkIdType numValues = outArray->GetNumberOfValues();
    numValues = inArray->GetNumberOfValues() < numValues ? inArray->GetNumberOfValues() : numValues;
    for (vtkIdType cc = 0; cc < numValues; ++cc)
    {
      TOut result = out[cc];
      switch (mode)
      {
        case vtkAttributeDataReductionFilter::ADD:
          result += in[cc];
          break;

        case vtkAttributeDataReductionFilter::MAX:
        {
          TIn v2 = in[cc];
          result = (result > v2) ? result : v2;
        }
        break;

        case vtkAttributeDataReductionFilter::MIN:
        {
          TIn v2 = in[cc];
          result = (result < v2) ? result : v2;
        }
        break;
      }
      out[cc] = result;
      self->UpdateProgress(progress_offset + progress_factor * cc / numValues);
    }
  }
};

//-----------------------------------------------------------------------------
void vtkAttributeDataReductionFilterReduce(vtkDataSetAttributes* output,
  std::vector<vtkDataSetAttributes*> inputs, vtkAttributeDataReductionFilter* self)
{
  int numInputs = static_cast<int>(inputs.size());
  assert(numInputs > 0);

  vtkDataSetAttributes* input0 = inputs[0];
  vtkIdType numTuples = input0->GetNumberOfTuples();

  vtkDataSetAttributes::FieldList fieldList;
  for (vtkDataSetAttributes* dsa : inputs)
  {
    // Include only field that have any arrays
    if (dsa->GetNumberOfArrays() > 0 && dsa->GetNumberOfTuples() == numTuples)
    {
      fieldList.IntersectFieldList(dsa);
    }
  }
  output->CopyGlobalIdsOn();
  output->CopyAllocate(fieldList, numTuples);
  // Copy 0th data over first.
  fieldList.CopyData(0, input0, 0, numTuples, output, 0);

  self->UpdateProgress(0.1);
  double progress_offset = 0.1;
  double progress_factor = (numInputs > 1) ? (0.9 * 0.5 / (numInputs - 1)) : 0;

  int list_index = 1;
  for (int cc = 1; cc < numInputs; ++cc)
  {
    vtkDataSetAttributes* dsa = inputs[cc];
    // Ignore if no arrays
    if (dsa->GetNumberOfArrays() > 0 && dsa->GetNumberOfTuples() == numTuples)
    {
      // Now combine this inPD with the outPD using the reduction indicated.
      auto f = [progress_offset, progress_factor, self](
                 vtkAbstractArray* fromA, vtkAbstractArray* toA)
      {
        vtkDataArray* toDA = vtkDataArray::SafeDownCast(toA);
        vtkDataArray* fromDA = vtkDataArray::SafeDownCast(fromA);
        if (!toDA || !fromDA)
        {
          return;
        }
        vtkAttributeDataReductionFilterReduceWorker worker;
        using Arrays = vtkTypeList::Append<vtkArrayDispatch::Arrays, vtkBitArray>::Result;
        if (!vtkArrayDispatch::Dispatch2ByArrayWithSameValueType<Arrays, Arrays>::Execute(
              fromDA, toDA, worker, self, progress_offset, progress_factor))
        {
          worker(fromDA, toDA, self, progress_offset, progress_factor);
        }
      };
      fieldList.TransformData(list_index, dsa, output, f);
      list_index++;
    }

    progress_offset += progress_factor;
  }
}
}

//-----------------------------------------------------------------------------
int vtkAttributeDataReductionFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* output = vtkDataObject::GetData(outputVector, 0);
  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  if (numInputs <= 0)
  {
    output->Initialize();
    return 0;
  }

  vtkDataObject* input0 = vtkDataObject::GetData(inputVector[0], 0);

  vtkDataSet* ds0 = vtkDataSet::SafeDownCast(input0);
  vtkDataSet* dsOutput = vtkDataSet::SafeDownCast(output);
  if (ds0 && dsOutput)
  {
    // Copy output structure from input0.
    dsOutput->CopyStructure(ds0);
  }

  vtkTable* table0 = vtkTable::SafeDownCast(input0);
  vtkTable* tableOutput = vtkTable::SafeDownCast(output);

  std::vector<vtkDataSetAttributes*> inputPDs;
  std::vector<vtkDataSetAttributes*> inputCDs;
  std::vector<vtkDataSetAttributes*> inputRDs;
  vtkSmartPointer<vtkDataSetAttributes> outputPD, outputCD, outputRD;

  if (dsOutput && ds0)
  {
    if ((this->AttributeType & vtkAttributeDataReductionFilter::POINT_DATA) == 0)
    {
      dsOutput->GetPointData()->ShallowCopy(ds0->GetCellData());
    }
    else
    {
      outputPD = dsOutput->GetPointData();
    }

    if ((this->AttributeType & vtkAttributeDataReductionFilter::CELL_DATA) == 0)
    {
      dsOutput->GetCellData()->ShallowCopy(ds0->GetCellData());
    }
    else
    {
      outputCD = dsOutput->GetCellData();
    }
  }

  if (tableOutput && table0)
  {
    if ((this->AttributeType & vtkAttributeDataReductionFilter::ROW_DATA) == 0)
    {
      tableOutput->GetRowData()->ShallowCopy(table0->GetRowData());
    }
    else
    {
      outputRD = tableOutput->GetRowData();
    }
  }

  for (int cc = 0; cc < numInputs; cc++)
  {
    vtkDataObject* inputCC = vtkDataObject::GetData(inputVector[0], cc);
    vtkTable* tableCC = vtkTable::SafeDownCast(inputCC);
    vtkDataSet* dsCC = vtkDataSet::SafeDownCast(inputCC);

    if ((this->AttributeType & vtkAttributeDataReductionFilter::POINT_DATA) && dsOutput && dsCC)
    {
      inputPDs.push_back(dsCC->GetPointData());
    }

    if ((this->AttributeType & vtkAttributeDataReductionFilter::CELL_DATA) && dsOutput && dsCC)
    {
      inputCDs.push_back(dsCC->GetCellData());
    }

    if ((this->AttributeType & vtkAttributeDataReductionFilter::ROW_DATA) && tableOutput && tableCC)
    {
      inputRDs.push_back(tableCC->GetRowData());
    }
  }

  if (outputPD)
  {
    ::vtkAttributeDataReductionFilterReduce(outputPD, inputPDs, this);
  }
  if (outputCD)
  {
    ::vtkAttributeDataReductionFilterReduce(outputCD, inputCDs, this);
  }
  if (outputRD)
  {
    ::vtkAttributeDataReductionFilterReduce(outputRD, inputRDs, this);
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkAttributeDataReductionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ReductionType: " << this->GetReductionTypeAsString() << endl;
  os << indent << "AttributeType: " << this->AttributeType << endl;
}
