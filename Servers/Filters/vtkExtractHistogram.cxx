/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractHistogram.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkExtractHistogram.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedLongArray.h"

#include <iostream>

vtkCxxRevisionMacro(vtkExtractHistogram, "1.1");
vtkStandardNewMacro(vtkExtractHistogram);

vtkExtractHistogram::vtkExtractHistogram() :
  Component(0),
  BinCount(10)
{
  this->SetInputArrayToProcess(
    0,
    0,
    0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS,
    vtkDataSetAttributes::SCALARS);
}

vtkExtractHistogram::~vtkExtractHistogram()
{
}

void vtkExtractHistogram::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Component: " << this->Component << "\n";
  os << indent << "BinCount: " << this->BinCount << "\n";
}

int vtkExtractHistogram::FillInputPortInformation (int port, vtkInformation *info)
{
  Superclass::FillInputPortInformation(port, info);
  
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

int vtkExtractHistogram::RequestData(vtkInformation* /*request*/, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDebugMacro(<< "Executing vtkExtractHistogram filter");

  // Build an empty output grid in advance, so we can bail-out if we encounter any problems
  vtkInformation* const output_info = outputVector->GetInformationObject(0);
  vtkPolyData* const output_data = vtkPolyData::SafeDownCast(
    output_info->Get(vtkDataObject::DATA_OBJECT()));

  vtkDoubleArray* const bin_extents = vtkDoubleArray::New();
  bin_extents->SetNumberOfComponents(1);
  bin_extents->SetNumberOfTuples(this->BinCount + 1);
  bin_extents->SetName("bin_extents");
  for(int i = 0; i != this->BinCount + 1; ++i)
    {
    bin_extents->SetValue(i, 0);
    }

  output_data->GetCellData()->AddArray(bin_extents);
  bin_extents->Delete();
  
  // Find the field to process, if we can't find anything, we return an empty dataset
  vtkDataArray* const data_array = this->GetInputArrayToProcess(0, inputVector);
  if(!data_array)
    {
    return 1;
    }

  // If the requested component is out-of-range for the input, we return an empty dataset
  if(this->Component < 0 || this->Component >= data_array->GetNumberOfComponents())
    {
    return 1;
    }

  // Calculate the extents of each bin, based on the range of values in the input ...
  // we offset the first and last values in the range by epsilon to ensure that
  // extrema don't fall "outside" the first or last bins due to errors in floating-point precision.
  double range[2];
  data_array->GetRange(range, this->Component);
  const double bin_delta = (range[1] - range[0]) / this->BinCount;
  
  bin_extents->SetValue(0, range[0] - VTK_DBL_EPSILON);
  for(int i = 1; i < this->BinCount; ++i)
    {
    bin_extents->SetValue(i, range[0] + (i * bin_delta));
    }
  bin_extents->SetValue(this->BinCount, range[1] + VTK_DBL_EPSILON);

  // Insert values into bins ...
  vtkUnsignedLongArray* const bin_values = vtkUnsignedLongArray::New();
  bin_values->SetNumberOfComponents(1);
  bin_values->SetNumberOfTuples(this->BinCount);
  bin_values->SetName("bin_values");
  
  for(int i = 0; i != this->BinCount; ++i)
    {
    bin_values->SetValue(i, 0);
    }
  
  for(int i = 0; i != data_array->GetNumberOfTuples(); ++i)
    {
    const double value = data_array->GetComponent(i, this->Component);
    for(int j = 0; j != this->BinCount; ++j)
      {
      if(bin_extents->GetValue(j) <= value && value < bin_extents->GetValue(j+1))
        {
        bin_values->SetValue(j, bin_values->GetValue(j) + 1);
        break;
        }
      }
    }

  output_data->GetCellData()->AddArray(bin_values);
  bin_values->Delete();

  return 1;
}
