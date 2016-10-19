/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractScatterPlot.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkExtractScatterPlot.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedLongArray.h"

#include "vtkIOStream.h"

vtkStandardNewMacro(vtkExtractScatterPlot);

vtkExtractScatterPlot::vtkExtractScatterPlot()
  : XComponent(0)
  , YComponent(0)
  , XBinCount(10)
  , YBinCount(10)
{
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::SCALARS);

  this->SetInputArrayToProcess(
    1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::SCALARS);
}

vtkExtractScatterPlot::~vtkExtractScatterPlot()
{
}

void vtkExtractScatterPlot::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "XComponent: " << this->XComponent << "\n";
  os << indent << "YComponent: " << this->YComponent << "\n";
  os << indent << "XBinCount: " << this->XBinCount << "\n";
  os << indent << "YBinCount: " << this->YBinCount << "\n";
}

int vtkExtractScatterPlot::FillInputPortInformation(int port, vtkInformation* info)
{
  Superclass::FillInputPortInformation(port, info);

  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

int vtkExtractScatterPlot::RequestData(vtkInformation* /*request*/,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int i, j;

  vtkDebugMacro(<< "Executing vtkExtractScatterPlot filter");

  // Build an empty output grid in advance, so we can bail-out if we
  // encounter any problems
  vtkInformation* const output_info = outputVector->GetInformationObject(0);
  vtkPolyData* const output_data =
    vtkPolyData::SafeDownCast(output_info->Get(vtkDataObject::DATA_OBJECT()));

  vtkDoubleArray* const x_bin_extents = vtkDoubleArray::New();
  x_bin_extents->SetNumberOfComponents(1);
  x_bin_extents->SetNumberOfTuples(this->XBinCount + 1);
  x_bin_extents->SetName("x_bin_extents");
  for (i = 0; i != this->XBinCount + 1; ++i)
  {
    x_bin_extents->SetValue(i, 0);
  }
  output_data->GetCellData()->AddArray(x_bin_extents);
  x_bin_extents->Delete();

  vtkDoubleArray* const y_bin_extents = vtkDoubleArray::New();
  y_bin_extents->SetNumberOfComponents(1);
  y_bin_extents->SetNumberOfTuples(this->XBinCount + 1);
  y_bin_extents->SetName("y_bin_extents");
  for (i = 0; i != this->YBinCount + 1; ++i)
  {
    y_bin_extents->SetValue(i, 0);
  }
  output_data->GetCellData()->AddArray(y_bin_extents);
  y_bin_extents->Delete();

  // Find the field to process, if we can't find anything, we return an
  // empty dataset
  vtkDataArray* const x_data_array = this->GetInputArrayToProcess(0, inputVector);
  if (!x_data_array)
  {
    return 1;
  }
  // If the requested component is out-of-range for the input, we return an
  // empty dataset
  if (this->XComponent < 0 || this->XComponent >= x_data_array->GetNumberOfComponents())
  {
    return 1;
  }

  // Find the field to process, if we can't find anything, we return an
  // empty dataset
  vtkDataArray* const y_data_array = this->GetInputArrayToProcess(1, inputVector);
  if (!y_data_array)
  {
    return 1;
  }
  // If the requested component is out-of-range for the input, we return an
  // empty dataset
  if (this->YComponent < 0 || this->YComponent >= y_data_array->GetNumberOfComponents())
  {
    return 1;
  }

  // Both fields better have the same number of tuples!
  if (x_data_array->GetNumberOfTuples() != y_data_array->GetNumberOfTuples())
  {
    return 1;
  }

  // Calculate the extents of each bin, based on the range of values in the
  // input ...  we offset the first and last values in each range by
  // epsilon to ensure that extrema don't fall "outside" the first or last
  // bins due to errors in floating-point precision.
  double x_range[2];
  x_data_array->GetRange(x_range, this->XComponent);
  const double x_bin_delta = (x_range[1] - x_range[0]) / this->XBinCount;

  x_bin_extents->SetValue(0, x_range[0] - VTK_DBL_EPSILON);
  for (i = 1; i < this->XBinCount; ++i)
  {
    x_bin_extents->SetValue(i, x_range[0] + (i * x_bin_delta));
  }
  x_bin_extents->SetValue(this->XBinCount, x_range[1] + VTK_DBL_EPSILON);

  double y_range[2];
  y_data_array->GetRange(y_range, this->YComponent);
  const double y_bin_delta = (y_range[1] - y_range[0]) / this->YBinCount;

  y_bin_extents->SetValue(0, y_range[0] - VTK_DBL_EPSILON);
  for (i = 1; i < this->YBinCount; ++i)
  {
    y_bin_extents->SetValue(i, y_range[0] + (i * y_bin_delta));
  }
  y_bin_extents->SetValue(this->YBinCount, y_range[1] + VTK_DBL_EPSILON);

  // Insert values into bins ...
  vtkUnsignedLongArray* const bin_values = vtkUnsignedLongArray::New();
  bin_values->SetNumberOfComponents(this->YBinCount);
  bin_values->SetNumberOfTuples(this->XBinCount);
  bin_values->SetName("bin_values");

  for (i = 0; i != this->XBinCount; ++i)
  {
    for (j = 0; j != this->YBinCount; ++j)
    {
      bin_values->SetComponent(i, j, 0);
    }
  }

  const int value_count = x_data_array->GetNumberOfTuples();
  for (i = 0; i != value_count; ++i)
  {
    const double x = x_data_array->GetComponent(i, this->XComponent);
    const double y = y_data_array->GetComponent(i, this->YComponent);

    for (j = 0; j != this->XBinCount; ++j)
    {
      if (x_bin_extents->GetValue(j) <= x && x < x_bin_extents->GetValue(j + 1))
      {
        for (int k = 0; k != this->YBinCount; ++k)
        {
          if (y_bin_extents->GetValue(k) <= y && y < y_bin_extents->GetValue(k + 1))
          {
            bin_values->SetComponent(j, k, bin_values->GetComponent(j, k) + 1);
            break;
          }
        }
        break;
      }
    }
  }

  output_data->GetCellData()->AddArray(bin_values);
  bin_values->Delete();

  return 1;
}
