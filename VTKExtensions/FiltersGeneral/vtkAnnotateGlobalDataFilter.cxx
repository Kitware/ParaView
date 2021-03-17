/*=========================================================================

  Program:   ParaView
  Module:    vtkAnnotateGlobalDataFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAnnotateGlobalDataFilter.h"

#include "vtkArrayDispatch.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataSetRange.h"
#include "vtkDataArray.h"
#include "vtkDataArrayAccessor.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <iterator>
#include <sstream>
#include <string>

#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

namespace
{

vtkFieldData* GetFieldData(vtkDataObject* dobj)
{
  auto fd = dobj->GetFieldData();
  if (fd->GetNumberOfArrays() > 0)
  {
    return fd;
  }
  if (auto cd = vtkCompositeDataSet::SafeDownCast(dobj))
  {
    using Opts = vtk::CompositeDataSetOptions;
    for (auto node : vtk::Range(cd, Opts::SkipEmptyNodes))
    {
      if (node && node->GetFieldData() && node->GetFieldData()->GetNumberOfArrays() > 0)
      {
        return node->GetFieldData();
      }
    }
  }
  return fd;
}

vtkIdType GetMode(vtkFieldData* fd, vtkAbstractArray* array)
{
  auto mode_shape = vtkIntArray::SafeDownCast(fd->GetArray("mode_shape"));
  auto mode_shape_range = vtkIntArray::SafeDownCast(fd->GetArray("mode_shape_range"));
  if (mode_shape_range && mode_shape_range->GetNumberOfTuples() == 1 &&
    mode_shape_range->GetNumberOfComponents() == 2 && mode_shape &&
    mode_shape->GetNumberOfComponents() == 1 && mode_shape->GetNumberOfTuples() == 1)
  {
    auto num_modes = static_cast<vtkIdType>(
      mode_shape_range->GetTypedComponent(0, 1) - mode_shape_range->GetTypedComponent(0, 0) + 1);
    if (array->GetNumberOfTuples() == num_modes)
    {
      vtkIdType mode =
        mode_shape->GetTypedComponent(0, 0) - mode_shape_range->GetTypedComponent(0, 0);
      return mode;
    }
  }
  return -1;
}

struct Printer
{
  vtkAnnotateGlobalDataFilter* Self;
  vtkIdType ChosenTuple;
  vtkStringArray* OutputArray;

  Printer(vtkIdType chosenTuple, vtkStringArray* outputA, vtkAnnotateGlobalDataFilter* self)
    : Self(self)
    , ChosenTuple(chosenTuple)
    , OutputArray(outputA)
  {
  }

  std::array<char, 4> StringTypeFormats = { 'c', 'C', 's', 'S' };
  std::array<char, 2> StringTypes = { VTK_STRING, VTK_UNICODE_STRING };
  std::array<char, 8> FloatTypeFormats = { 'a', 'A', 'e', 'E', 'f', 'F', 'g', 'G' };
  std::array<int, 2> FloatTypes = { VTK_FLOAT, VTK_DOUBLE };
  std::array<char, 6> IntegralTypeFormats = { 'd', 'i', 'o', 'u', 'x', 'X' };
  std::array<char, 12> IntegralTypes = { VTK_CHAR, VTK_SIGNED_CHAR, VTK_UNSIGNED_CHAR, VTK_SHORT,
    VTK_UNSIGNED_SHORT, VTK_INT, VTK_UNSIGNED_INT, VTK_LONG, VTK_UNSIGNED_LONG, VTK_ID_TYPE,
    VTK_LONG_LONG, VTK_UNSIGNED_LONG_LONG };

  // Validate that a format string is valid for a given data type
  bool IsFormatStringValid(const char* format, int typeID)
  {
    vtksys::RegularExpression regex(
      "(%([-+0 #]?[-+0 #]?[-+0 #]?[-+0 #]?[-+0 "
      "#]?)([0-9]+)?(\\.([0-9]+))?(h|l|ll|w|II32|I64)?([cCdiouxXeEfgGaAnpsS])|%%)");

    regex.find(format);
    std::string typeString = regex.match(7);

    bool validType = false;
    if (std::find(this->StringTypes.begin(), this->StringTypes.end(), typeID) !=
      this->StringTypes.end())
    {
      validType = std::find(this->StringTypeFormats.begin(), this->StringTypeFormats.end(),
                    typeString[0]) != this->StringTypeFormats.end();
    }
    else if (std::find(this->FloatTypes.begin(), this->FloatTypes.end(), typeID) !=
      this->FloatTypes.end())
    {
      validType = std::find(this->FloatTypeFormats.begin(), this->FloatTypeFormats.end(),
                    typeString[0]) != this->FloatTypeFormats.end();
    }
    else if (std::find(this->IntegralTypes.begin(), this->IntegralTypes.end(), typeID) !=
      this->IntegralTypes.end())
    {
      validType = std::find(this->IntegralTypeFormats.begin(), this->IntegralTypeFormats.end(),
                    typeString[0]) != this->IntegralTypeFormats.end();
    }

    return validType;
  }

  template <typename ArrayT>
  void operator()(ArrayT* array)
  {
    assert(this->ChosenTuple >= 0 && this->ChosenTuple < array->GetNumberOfTuples());

    auto self = this->Self;
    int dataType = array->GetDataType();
    bool formatValid = this->IsFormatStringValid(self->GetFormat(), dataType);

    std::ostringstream stream;
    stream << (self->GetPrefix() ? self->GetPrefix() : "");

    if (formatValid)
    {
      char buffer[256];
      vtkDataArrayAccessor<ArrayT> accessor(array);
      const auto numComps = array->GetNumberOfComponents();
      if (numComps == 1)
      {
        std::snprintf(buffer, 256, self->GetFormat(), accessor.Get(this->ChosenTuple, 0));
        stream << buffer;
      }
      else if (numComps > 1)
      {
        stream << "(";
        for (int cc = 0; cc < numComps; ++cc)
        {
          std::snprintf(buffer, 256, self->GetFormat(), accessor.Get(this->ChosenTuple, cc));
          stream << (cc > 0 ? ", " : " ");
          stream << buffer;
        }
        stream << " )";
      }
    }
    else
    {
      vtkGenericWarningMacro("Format string '" << self->GetFormat()
                                               << "' is not valid for data array type "
                                               << array->GetDataTypeAsString());
      stream << "(error)";
    }

    stream << (self->GetPostfix() ? self->GetPostfix() : "");
    this->OutputArray->SetValue(0, stream.str());
  }

  void operator()(vtkStringArray* array)
  {
    assert(this->ChosenTuple >= 0 && this->ChosenTuple < array->GetNumberOfTuples());

    auto self = this->Self;
    int dataType = array->GetDataType();
    bool formatValid = this->IsFormatStringValid(self->GetFormat(), dataType);

    std::ostringstream stream;
    stream << (self->GetPrefix() ? self->GetPrefix() : "");

    if (formatValid)
    {
      char buffer[256];
      const auto numComps = array->GetNumberOfComponents();
      if (numComps == 1)
      {
        std::snprintf(buffer, 256, self->GetFormat(), array->GetValue(this->ChosenTuple).c_str());
        stream << buffer;
      }
      else if (numComps > 1)
      {
        stream << "(";
        for (int cc = 0; cc < numComps; ++cc)
        {
          std::snprintf(buffer, 256, self->GetFormat(),
            array->GetValue(this->ChosenTuple * numComps + cc).c_str());
          stream << (cc > 0 ? ", " : " ");
          stream << buffer;
        }
        stream << " )";
      }
    }
    else
    {
      vtkGenericWarningMacro("Format string '" << self->GetFormat()
                                               << "' is not valid for data array type "
                                               << array->GetDataTypeAsString());
      stream << "(error)";
    }

    stream << (self->GetPostfix() ? self->GetPostfix() : "");
    this->OutputArray->SetValue(0, stream.str());
  }
};
}

vtkStandardNewMacro(vtkAnnotateGlobalDataFilter);
vtkCxxSetObjectMacro(vtkAnnotateGlobalDataFilter, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkAnnotateGlobalDataFilter::vtkAnnotateGlobalDataFilter()
  : Prefix(nullptr)
  , Postfix(nullptr)
  , FieldArrayName(nullptr)
  , Format(nullptr)
  , Controller(nullptr)
{
  this->SetFormat("%7.5g");
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkAnnotateGlobalDataFilter::~vtkAnnotateGlobalDataFilter()
{
  this->SetPrefix(nullptr);
  this->SetPostfix(nullptr);
  this->SetFieldArrayName(nullptr);
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
int vtkAnnotateGlobalDataFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkAnnotateGlobalDataFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (this->Controller && this->Controller->GetLocalProcessId() > 0)
  {
    return 1;
  }

  auto table = vtkTable::GetData(outputVector, 0);

  vtkNew<vtkStringArray> sarray;
  sarray->SetName("Text");
  sarray->SetNumberOfComponents(1);
  sarray->SetNumberOfTuples(1);
  sarray->SetValue(0, "(error)");
  table->AddColumn(sarray);

  if (this->FieldArrayName == nullptr || this->FieldArrayName[0] == '\0')
  {
    vtkErrorMacro("No FieldArrayName specified!");
    return 1;
  }

  auto inInfo = inputVector[0]->GetInformationObject(0);
  auto input = vtkDataObject::GetData(inInfo);
  auto fd = ::GetFieldData(input);
  if (!fd)
  {
    vtkErrorMacro("Missing field data!");
    return 1;
  }

  auto array = fd->GetAbstractArray(this->FieldArrayName);
  if (array == nullptr)
  {
    vtkErrorMacro("Failed to locate array '" << this->FieldArrayName << "'.");
    return 1;
  }

  vtkIdType chosenTuple = 0;
  double data_time = VTK_DOUBLE_MAX;
  if (input->GetInformation()->Has(vtkDataObject::DATA_TIME_STEP()))
  {
    data_time = input->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
  }

  // if the array has as many elements as the timesteps, pick the element
  // matching the current timestep.
  if (inInfo && inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    auto num_timesteps = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    auto timesteps = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    if (num_timesteps > 0 && array->GetNumberOfTuples() == num_timesteps &&
      data_time != VTK_DOUBLE_MAX)
    {
      auto iter = std::find(timesteps, timesteps + num_timesteps, data_time);
      if (iter != timesteps + num_timesteps)
      {
        chosenTuple = static_cast<vtkIdType>(std::distance(timesteps, iter));
      }
    }
  }
  // if the array has as many elements as the `mode_shape_range`, pick the
  // element matching the `mode_shape` (BUG #0015322).
  else
  {
    auto mode = ::GetMode(fd, array);
    if (mode != -1)
    {
      chosenTuple = mode;
    }
  }

  if (array->GetNumberOfTuples() <= chosenTuple)
  {
    vtkErrorMacro("Incorrect tuple '" << chosenTuple << "'. Array only has "
                                      << array->GetNumberOfTuples() << " tuples.");
    return 1;
  }

  using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;
  Printer printer(chosenTuple, sarray, this);
  auto da = vtkDataArray::SafeDownCast(array);
  if (da && !Dispatcher::Execute(da, printer))
  {
    printer(da);
  }
  else if (auto sa = vtkStringArray::SafeDownCast(array))
  {
    printer(sa);
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkAnnotateGlobalDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldArrayName: " << (this->FieldArrayName ? this->FieldArrayName : "(none)")
     << endl;
  os << indent << "Prefix: " << (this->Prefix ? this->Prefix : "(none)") << endl;
  os << indent << "Postfix: " << (this->Postfix ? this->Postfix : "(none)") << endl;
}
