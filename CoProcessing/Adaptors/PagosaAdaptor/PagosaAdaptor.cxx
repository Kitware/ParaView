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
#include "FortranAdaptorAPI.h"
#include "FortranPythonAdaptorAPI.h"

#include "vtkCPAdaptorAPI.h"
#include "vtkCPDataDescription.h"
#include "vtkCPInputDataDescription.h"
#include "vtkCPProcessor.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataSet.h"
#include <iostream>

#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPointData.h"

#include "vtkMultiBlockDataSet.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

/*
 * Pagosa is a simulation code. It is a closed source code. A copy of the Physics
 * Manual is at http://permalink.lanl.gov/object/tr?what=info:lanl-repo/lareport/LA-14425-M
 */

namespace
{
int numberOfMarkers;
};

///////////////////////////////////////////////////////////////////////////////
//
// Define the data structures to hold the in situ output VTK data
// vtkNonOverlappingAMR is required for using the MaterialInterface filter
// vtkUnstructuredGrid will hold the data currently written to .cosmo files
//
///////////////////////////////////////////////////////////////////////////////

extern "C" void setcoprocessorgeometry_(int* mx, int* my, int* mz, double* x0, double* y0,
  double* z0, double* dx, double* dy, double* dz, unsigned int* my_id, const int* tot_pes)
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

///////////////////////////////////////////////////////////////////////////////
//
// Set a field in the first grid of nonoverlapping AMR
//
///////////////////////////////////////////////////////////////////////////////

extern "C" void setcoprocessorfield_(char* fname, int* len, int* mx, int* my, int* mz,
  unsigned int* my_id, float* data, bool* down_convert)
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
    vtkStdString name(fname, *len);
    att->SetName(name);
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
    vtkStdString name(fname, *len);
    att->SetName(name);
    att->SetNumberOfComponents(1);
    att->SetArray(data, numCells, 1);
    img->GetCellData()->AddArray(att);
  }

  img->GetCellData()->Modified();
  grid->Modified();
}

///////////////////////////////////////////////////////////////////////////////
//
// Initialize unstructured grid for markers and allocate size
//
///////////////////////////////////////////////////////////////////////////////

extern "C" void setmarkergeometry_(int* nvp, int* my_id)
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

///////////////////////////////////////////////////////////////////////////////
//
// Add a field to the unstructured grid of markers
//
///////////////////////////////////////////////////////////////////////////////

extern "C" void addmarkergeometry_(int* numberOfParticles, // Particles in added field
  float* xloc,                                             // Location
  float* yloc,                                             // Location
  float* zloc)                                             // Location
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

///////////////////////////////////////////////////////////////////////////////
//
// Set a scalar field in the unstructured grid of markers
//
///////////////////////////////////////////////////////////////////////////////

extern "C" void addmarkerscalarfield_(char* fname, // Name of data
  int* len,                                        // Length of data name
  int* numberOfParticles,                          // Particles in field
  float* data)                                     // Data by particle
{
  vtkMultiBlockDataSet* mgrid = vtkMultiBlockDataSet::SafeDownCast(
    vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("input2")->GetGrid());
  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(mgrid->GetBlock(0));

  // Get the data array for this variable
  vtkStdString varName(fname, *len);
  vtkFloatArray* dataArray = vtkFloatArray::SafeDownCast(ugrid->GetPointData()->GetArray(varName));

  // If it doesn't exist, create and size, and refetch
  if (dataArray == nullptr)
  {
    vtkFloatArray* arr = vtkFloatArray::New();
    arr->SetName(varName);
    arr->SetNumberOfComponents(1);
    arr->Allocate(numberOfMarkers);
    ugrid->GetPointData()->AddArray(arr);
    arr->Delete();
    dataArray = vtkFloatArray::SafeDownCast(ugrid->GetPointData()->GetArray(varName));
  }

  // Fill with field data
  for (int i = 0; i < *numberOfParticles; i++)
  {
    dataArray->InsertNextValue((float)data[i]);
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Set a vector field in the unstructured grid of markers
//
///////////////////////////////////////////////////////////////////////////////

extern "C" void addmarkervectorfield_(char* fname, // Name of data
  int* len,                                        // Length of data name
  int* numberOfParticles,                          // Particles in field
  float* data0,                                    // Data by particle
  float* data1,                                    // Data by particle
  float* data2)                                    // Data by particle
{
  vtkMultiBlockDataSet* mgrid = vtkMultiBlockDataSet::SafeDownCast(
    vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("input2")->GetGrid());
  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(mgrid->GetBlock(0));

  // Get the data array for this variable
  vtkStdString varName(fname, *len);
  vtkFloatArray* dataArray = vtkFloatArray::SafeDownCast(ugrid->GetPointData()->GetArray(varName));

  // If it doesn't exist, create and size, and refetch
  if (dataArray == nullptr)
  {
    vtkFloatArray* arr = vtkFloatArray::New();
    arr->SetName(varName);
    arr->SetNumberOfComponents(3);
    arr->Allocate(numberOfMarkers);
    ugrid->GetPointData()->AddArray(arr);
    arr->Delete();
    dataArray = vtkFloatArray::SafeDownCast(ugrid->GetPointData()->GetArray(varName));
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

///////////////////////////////////////////////////////////////////////////////
//
// Set a 6 element tensor field in the unstructured grid of markers
//
///////////////////////////////////////////////////////////////////////////////

extern "C" void addmarkertensorfield_(char* fname, // Name of data
  int* len,                                        // Length of data name
  int* numberOfParticles,                          // Particles in field
  float* data0,                                    // Data by particle
  float* data1,                                    // Data by particle
  float* data2,                                    // Data by particle
  float* data3,                                    // Data by particle
  float* data4,                                    // Data by particle
  float* data5)                                    // Data by particle
{
  vtkMultiBlockDataSet* mgrid = vtkMultiBlockDataSet::SafeDownCast(
    vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("input2")->GetGrid());
  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(mgrid->GetBlock(0));

  // Get the data array for this variable
  vtkStdString varName(fname, *len);
  vtkFloatArray* dataArray = vtkFloatArray::SafeDownCast(ugrid->GetPointData()->GetArray(varName));

  // If it doesn't exist, create and size, and refetch
  if (dataArray == nullptr)
  {
    vtkFloatArray* arr = vtkFloatArray::New();
    arr->SetName(varName);
    arr->SetNumberOfComponents(6);
    arr->Allocate(numberOfMarkers);
    ugrid->GetPointData()->AddArray(arr);
    arr->Delete();
    dataArray = vtkFloatArray::SafeDownCast(ugrid->GetPointData()->GetArray(varName));
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
