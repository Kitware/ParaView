/*=========================================================================

  Program:   ParaView
  Module:    PagosaAdaptor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "PagosaAdaptor.h"

#include "FortranAdaptorAPI.h"
#include "vtkCPAdaptorAPI.h"
#include "vtkCPDataDescription.h"
#include "vtkCPInputDataDescription.h"
#include "vtkCPProcessor.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkPointData.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <iostream>

#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
#include "FortranPythonAdaptorAPI.h"
#endif

namespace
{
int numberOfMarkers;
};

//------------------------------------------------------------------------------
void setcoprocessorgeometry_(int* mx, int* my, int* mz, double* x0, double* y0, double* z0,
  double* dx, double* dy, double* dz, unsigned int* my_id, const int* tot_pes)
{
  if (!vtkCPAdaptorAPI::GetCoProcessorData())
  {
    vtkGenericWarningMacro("Unable to access CoProcessorData.");
    return;
  }

  // Setup the uniform grid which is put into the nonoverlapping AMR
  const int ndim[3] = { *mx + 2, *my + 2, *mz + 2 };
  double origin[3] = { *x0, *y0, *z0 };
  double spacing[3] = { *dx, *dy, *dz };

  vtkUniformGrid* img = vtkUniformGrid::New();
  img->SetDimensions(ndim);
  img->SetOrigin(origin);
  img->SetSpacing(spacing);

  // Nonoverlapping AMR has just one level and a block per processor
  vtkNonOverlappingAMR* grid = vtkNonOverlappingAMR::New();
  int numLevels = 1;
  const int blocksPerLevel[1] = { *tot_pes };
  grid->Initialize(numLevels, blocksPerLevel);
  grid->SetGridDescription(VTK_XYZ_GRID);
  unsigned int level = 0;
  unsigned int blockId = *my_id;
  grid->SetDataSet(level, blockId, img);

  // Unstructured grid will hold the marker information currently in .cosmo
  vtkMultiBlockDataSet* mgrid = vtkMultiBlockDataSet::New();
  mgrid->SetNumberOfBlocks(1);

  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::New();
  ugrid->Initialize();

  mgrid->SetBlock(0, ugrid);

  // Add both grids to the coprocessor
  vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("input")->SetGrid(grid);
  vtkCPAdaptorAPI::GetCoProcessorData()->AddInput("input2");
  vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("input2")->SetGrid(mgrid);

  ugrid->Delete();
}

//------------------------------------------------------------------------------
void setcoprocessorfield_(char* fname, int* len, int* mx, int* my, int* mz, unsigned int* my_id,
  float* data, bool* down_convert)
{
  vtkNonOverlappingAMR* grid = vtkNonOverlappingAMR::SafeDownCast(
    vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("input")->GetGrid());

  unsigned int level = 0;
  unsigned int blockId = *my_id;

  vtkUniformGrid* img = vtkUniformGrid::SafeDownCast(grid->GetDataSet(level, blockId));
  vtkIdType numCells = (*mx + 1) * (*my + 1) * (*mz + 1);

  if (*down_convert)
  {
    vtkNew<vtkUnsignedCharArray> att;
    std::string name(fname, *len);
    att->SetName(name.c_str());
    att->SetNumberOfComponents(1);
    att->SetNumberOfTuples(numCells);
    for (vtkIdType idx = 0; idx < numCells; ++idx)
    {
      att->SetValue(idx, static_cast<unsigned char>(data[idx] * 255.0));
    }
    img->GetCellData()->AddArray(att);
  }
  else
  {
    vtkNew<vtkFloatArray> att;
    std::string name(fname, *len);
    att->SetName(name.c_str());
    att->SetNumberOfComponents(1);
    att->SetNumberOfTuples(numCells);
    for (vtkIdType idx = 0; idx < numCells; ++idx)
    {
      att->SetValue(idx, data[idx]);
    }
    img->GetCellData()->AddArray(att);
  }

  img->GetCellData()->Modified();
  grid->Modified();
}

//------------------------------------------------------------------------------
void setmarkergeometry_(int* nvp, int*)
{
  vtkMultiBlockDataSet* mgrid = vtkMultiBlockDataSet::SafeDownCast(
    vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("input2")->GetGrid());
  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(mgrid->GetBlock(0));

  numberOfMarkers = *nvp;

  // Initialize any arrays in the current ugrid
  int numberOfArrays = ugrid->GetPointData()->GetNumberOfArrays();
  for (int i = 0; i < numberOfArrays; i++)
  {
    vtkFloatArray* dataArray = vtkFloatArray::SafeDownCast(ugrid->GetPointData()->GetArray(i));
    dataArray->Initialize();
  }

  // Set size of the grid back to empty
  ugrid->Initialize();

  // Allocate points and cells for all markers, but don't fill in
  vtkPoints* points = vtkPoints::New();
  points->SetDataTypeToFloat();
  points->Allocate(numberOfMarkers);

  ugrid->Allocate(numberOfMarkers);
  ugrid->SetPoints(points);

  points->Delete();
}

//------------------------------------------------------------------------------
void addmarkergeometry_(int* numberOfParticles, float* xloc, float* yloc, float* zloc)
{
  vtkMultiBlockDataSet* mgrid = vtkMultiBlockDataSet::SafeDownCast(
    vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("input2")->GetGrid());
  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(mgrid->GetBlock(0));

  // Get pointers to points and cells which were allocated but not inserted
  vtkPoints* points = ugrid->GetPoints(); // ACB is the points here appended for multiple calls?

  for (int i = 0; i < *numberOfParticles; i++)
  {
    float pt[3];
    pt[0] = xloc[i];
    pt[1] = yloc[i];
    pt[2] = zloc[i];
    vtkIdType pid = points->InsertNextPoint(pt);
    ugrid->InsertNextCell(VTK_VERTEX, 1, &pid);
  }
}

//------------------------------------------------------------------------------
void addmarkerscalarfield_(char* fname, int* len, int* numberOfParticles, float* data)
{
  vtkMultiBlockDataSet* mgrid = vtkMultiBlockDataSet::SafeDownCast(
    vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("input2")->GetGrid());
  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(mgrid->GetBlock(0));

  // Get the data array for this variable
  std::string varName(fname, *len);
  vtkFloatArray* dataArray =
    vtkFloatArray::SafeDownCast(ugrid->GetPointData()->GetArray(varName.c_str()));
  // If it doesn't exist, create and size, and refetch
  if (dataArray == nullptr)
  {
    vtkFloatArray* arr = vtkFloatArray::New();
    arr->SetName(varName.c_str());
    arr->SetNumberOfComponents(1);
    arr->Allocate(numberOfMarkers);
    ugrid->GetPointData()->AddArray(arr);
    arr->Delete();
    dataArray = vtkFloatArray::SafeDownCast(ugrid->GetPointData()->GetArray(varName.c_str()));
  }

  // Fill with field data
  for (int i = 0; i < *numberOfParticles; i++)
  {
    dataArray->InsertNextValue((float)data[i]);
  }
}

//------------------------------------------------------------------------------
void addmarkervectorfield_(
  char* fname, int* len, int* numberOfParticles, float* data0, float* data1, float* data2)
{
  vtkMultiBlockDataSet* mgrid = vtkMultiBlockDataSet::SafeDownCast(
    vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("input2")->GetGrid());
  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(mgrid->GetBlock(0));

  // Get the data array for this variable
  std::string varName(fname, *len);
  vtkFloatArray* dataArray =
    vtkFloatArray::SafeDownCast(ugrid->GetPointData()->GetArray(varName.c_str()));

  // If it doesn't exist, create and size, and refetch
  if (dataArray == nullptr)
  {
    vtkFloatArray* arr = vtkFloatArray::New();
    arr->SetName(varName.c_str());
    arr->SetNumberOfComponents(3);
    arr->Allocate(numberOfMarkers);
    ugrid->GetPointData()->AddArray(arr);
    arr->Delete();
    dataArray = vtkFloatArray::SafeDownCast(ugrid->GetPointData()->GetArray(varName.c_str()));
  }

  // Fill with field data
  for (int i = 0; i < *numberOfParticles; i++)
  {
    float tuple[3];
    tuple[0] = (float)data0[i];
    tuple[1] = (float)data1[i];
    tuple[2] = (float)data2[i];
    dataArray->InsertNextTuple(tuple);
  }
}

//------------------------------------------------------------------------------
void addmarkertensorfield_(char* fname, int* len, int* numberOfParticles, float* data0,
  float* data1, float* data2, float* data3, float* data4, float* data5)
{
  vtkMultiBlockDataSet* mgrid = vtkMultiBlockDataSet::SafeDownCast(
    vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("input2")->GetGrid());
  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(mgrid->GetBlock(0));

  // Get the data array for this variable
  std::string varName(fname, *len);
  vtkFloatArray* dataArray =
    vtkFloatArray::SafeDownCast(ugrid->GetPointData()->GetArray(varName.c_str()));

  // If it doesn't exist, create and size, and refetch
  if (dataArray == nullptr)
  {
    vtkFloatArray* arr = vtkFloatArray::New();
    arr->SetName(varName.c_str());
    arr->SetNumberOfComponents(6);
    arr->Allocate(numberOfMarkers);
    ugrid->GetPointData()->AddArray(arr);
    arr->Delete();
    dataArray = vtkFloatArray::SafeDownCast(ugrid->GetPointData()->GetArray(varName.c_str()));
  }

  // Fill with field data
  for (int i = 0; i < *numberOfParticles; i++)
  {
    float tuple[6];
    tuple[0] = (float)data0[i];
    tuple[1] = (float)data1[i];
    tuple[2] = (float)data2[i];
    tuple[3] = (float)data3[i];
    tuple[4] = (float)data4[i];
    tuple[5] = (float)data5[i];
    dataArray->InsertNextTuple(tuple);
  }
}
