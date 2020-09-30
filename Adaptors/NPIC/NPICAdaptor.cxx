/*=========================================================================

  Program:   ParaView
  Module:    NPICAdaptor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "NPICAdaptor.h"

#include "vtkCPAdaptorAPI.h"
#include "vtkCPDataDescription.h"
#include "vtkCPInputDataDescription.h"
#include "vtkCPProcessor.h"
#include "vtkCellType.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPointData.h"

//------------------------------------------------------------------------------
void createstructuredgrid_(
  int* myid, int* xdim, int* ystart, int* ystop, double* xspc, double* yspc)
{
  if (!vtkCPAdaptorAPI::GetCoProcessorData())
  {
    vtkGenericWarningMacro("Unable to access CoProcessorData.");
    return;
  }

  vtkMultiBlockDataSet* grid = vtkMultiBlockDataSet::New();
  grid->SetNumberOfBlocks(1);

  vtkImageData* img = vtkImageData::New();
  img->Initialize();

  grid->SetBlock(0, img);
  img->Delete();

  img->SetSpacing(*xspc, *yspc, 0.0);
  // Offsets account for a ghost point on either end in both directions
  // They also account for the 1 base in Fortran vs the 0 base here.
  img->SetExtent(-1, *xdim, ystart[*myid] - 2, ystop[*myid], 0, 0);
  img->SetOrigin(0.0, 0.0, 0.0);

  vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("input")->SetGrid(grid);
  grid->Delete();
}

//------------------------------------------------------------------------------
void add_scalar_(char* fname, int* len, double* data, int* size)
{
  vtkDoubleArray* arr = vtkDoubleArray::New();
  std::string name(fname, *len);
  arr->SetName(name.c_str());
  arr->SetNumberOfComponents(1);
  // arr->SetNumberOfTuples (*size);
  arr->SetArray(data, *size, 1);
  vtkMultiBlockDataSet* grid = vtkMultiBlockDataSet::SafeDownCast(
    vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("input")->GetGrid());
  vtkDataSet* dataset = vtkDataSet::SafeDownCast(grid->GetBlock(0));
  dataset->GetPointData()->AddArray(arr);
  arr->Delete();
}

//------------------------------------------------------------------------------
void add_vector_(char* fname, int* len, double* data0, double* data1, double* data2, int* size)
{
  vtkDoubleArray* arr = vtkDoubleArray::New();
  std::string name(fname, *len);
  arr->SetName(name.c_str());
  arr->SetNumberOfComponents(3);
  arr->SetNumberOfTuples(*size);
  for (int i = 0; i < *size; i++)
  {
    arr->SetComponent(i, 0, data0[i]);
    arr->SetComponent(i, 1, data1[i]);
    arr->SetComponent(i, 2, data2[i]);
  }
  vtkMultiBlockDataSet* grid = vtkMultiBlockDataSet::SafeDownCast(
    vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("input")->GetGrid());
  vtkDataSet* dataset = vtkDataSet::SafeDownCast(grid->GetBlock(0));
  dataset->GetPointData()->AddArray(arr);
  arr->Delete();
}

//------------------------------------------------------------------------------
void add_pressure_(int* index, double* data, int* size)
{
  static char Pe[3] = "Pe";
  static char Pi[3] = "Pi";
  static int componentMap[6] = { 0, 3, 5, 1, 4, 2 };
  vtkMultiBlockDataSet* grid = vtkMultiBlockDataSet::SafeDownCast(
    vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("input")->GetGrid());

  int real_index = *index - 24;
  char* name;
  if (real_index < 6)
  {
    name = Pe;
  }
  else
  {
    name = Pi;
    real_index -= 6;
  }

  // reorder the tensor components to paraview's assumption
  real_index = componentMap[real_index];

  int ignore;
  vtkDataSet* dataset = vtkDataSet::SafeDownCast(grid->GetBlock(0));
  vtkDoubleArray* arr =
    vtkDoubleArray::SafeDownCast(dataset->GetPointData()->GetArray(name, ignore));
  if (!arr)
  {
    arr = vtkDoubleArray::New();
    arr->SetName(name);
    arr->SetNumberOfComponents(6);
    arr->SetNumberOfTuples(*size);
    dataset->GetPointData()->AddArray(arr);
    arr->Delete();
  }

  for (int i = 0; i < *size; i++)
  {
    arr->SetComponent(i, real_index, data[i]);
  }
}
