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
#include "vtkNew.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkPointData.h"
#include "vtkStringArray.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <iostream>

#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
#include "FortranPythonAdaptorAPI.h"
#endif

#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

/*
 * Pagosa is a simulation code.  It is a closed source code.  A copy of the Physics
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

void setcoprocessorgeometry_(int* mx, int* my, int* mz, double* x0, double* y0, double* z0,
  double* dx, double* dy, double* dz, int* my_id, const int* tot_pes, char* nframe, int* nframelen,
  char* version, int* versionlen)
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

  // Add FieldData array for vline2 Annotation
  vtkStdString nframeStr(nframe, *nframelen);
  VTK_CREATE(vtkStringArray, nframeArray);
  nframeArray->SetName("Frame");
  nframeArray->SetNumberOfComponents(1);
  nframeArray->SetNumberOfTuples(1);
  nframeArray->SetValue(0, nframeStr);
  grid->GetFieldData()->AddArray(nframeArray);
  mgrid->GetFieldData()->AddArray(nframeArray);

  // Add FieldData array for the vline Annotation
  vtkStdString versionStr(version, *versionlen);
  VTK_CREATE(vtkStringArray, versionArray);
  versionArray->SetName("Version");
  versionArray->SetNumberOfComponents(1);
  versionArray->SetNumberOfTuples(1);
  versionArray->SetValue(0, versionStr);
  grid->GetFieldData()->AddArray(versionArray);
  mgrid->GetFieldData()->AddArray(versionArray);

  // Add FieldData array for cycle number annotation
  VTK_CREATE(vtkIntArray, cycleArray);
  cycleArray->SetName("CycleNumber");
  cycleArray->SetNumberOfComponents(1);
  cycleArray->SetNumberOfTuples(1);
  cycleArray->SetTuple1(0, 0);
  grid->GetFieldData()->AddArray(cycleArray);
  mgrid->GetFieldData()->AddArray(cycleArray);

  // Add FieldData array for simulation time annotation
  VTK_CREATE(vtkFloatArray, simTimeArray);
  simTimeArray->SetName("SimulationTime");
  simTimeArray->SetNumberOfComponents(1);
  simTimeArray->SetNumberOfTuples(1);
  simTimeArray->SetTuple1(0, 0.0);
  grid->GetFieldData()->AddArray(simTimeArray);
  mgrid->GetFieldData()->AddArray(simTimeArray);

  // Add both grids to the coprocessor
  vtkCPAdaptorAPI::GetCoProcessorData()->AddInput("GridCellData");
  vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("GridCellData")->SetGrid(grid);
  vtkCPAdaptorAPI::GetCoProcessorData()->AddInput("MarkerPointData");
  vtkCPAdaptorAPI::GetCoProcessorData()
    ->GetInputDescriptionByName("MarkerPointData")
    ->SetGrid(mgrid);

  ugrid->Delete();
}

///////////////////////////////////////////////////////////////////////////////
//
// Update the vtkNonOverlappingAMR for every time step
// This holds ImageData which does not change size, but frame, version, cycle and simulation time
// change
//
///////////////////////////////////////////////////////////////////////////////

void setgridgeometry_(
  char* nframe, int* nframelen, char* version, int* versionlen, int* cycleNum, double* simTime)
{
  vtkNonOverlappingAMR* grid = vtkNonOverlappingAMR::SafeDownCast(
    vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("GridCellData")->GetGrid());

  // Set FieldData array for vline2 annotation
  vtkStdString nframeStr(nframe, *nframelen);
  vtkStringArray* nframeArray =
    vtkStringArray::SafeDownCast(grid->GetFieldData()->GetAbstractArray("Frame"));
  nframeArray->SetValue(0, nframeStr);

  // Set FieldData array for vline annotation
  vtkStdString versionStr(version, *versionlen);
  vtkStringArray* versionArray =
    vtkStringArray::SafeDownCast(grid->GetFieldData()->GetAbstractArray("Version"));
  versionArray->SetValue(0, versionStr);

  // Set FieldData for CycleNumber TimeArray
  vtkIntArray* cycleArray =
    vtkIntArray::SafeDownCast(grid->GetFieldData()->GetArray("CycleNumber"));
  cycleArray->SetTuple1(0, *cycleNum);

  // Set FieldData for SimulationTime TimeArray
  vtkFloatArray* simTimeArray =
    vtkFloatArray::SafeDownCast(grid->GetFieldData()->GetArray("SimulationTime"));
  simTimeArray->SetTuple1(0, (float)*simTime);
}

///////////////////////////////////////////////////////////////////////////////
//
// Set a field in the first grid of nonoverlapping AMR
//
///////////////////////////////////////////////////////////////////////////////

void addgridfield_(
  char* fname, int* len, int* mx, int* my, int* mz, int* my_id, float* data, bool* down_convert)
{
  vtkNonOverlappingAMR* grid = vtkNonOverlappingAMR::SafeDownCast(
    vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("GridCellData")->GetGrid());

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

///////////////////////////////////////////////////////////////////////////////
//
// Initialize unstructured grid for markers, allocate size for all markers
// across all processors, and update annotation data.
//
///////////////////////////////////////////////////////////////////////////////

void setmarkergeometry_(int* nvp, char* nframe, int* nframelen, char* version, int* versionlen,
  int* cycleNum, double* simTime)
{
  vtkMultiBlockDataSet* mgrid = vtkMultiBlockDataSet::SafeDownCast(
    vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("MarkerPointData")->GetGrid());

  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(mgrid->GetBlock(0));

  // Set FieldData array for vline2 annotation
  vtkStdString nframeStr(nframe, *nframelen);
  vtkStringArray* nframeArray =
    vtkStringArray::SafeDownCast(mgrid->GetFieldData()->GetAbstractArray("Frame"));
  nframeArray->SetValue(0, nframeStr);

  // Set FieldData array for vline annotation
  vtkStdString versionStr(version, *versionlen);
  vtkStringArray* versionArray =
    vtkStringArray::SafeDownCast(mgrid->GetFieldData()->GetAbstractArray("Version"));
  versionArray->SetValue(0, versionStr);

  // Set FieldData for CycleIndex TimeArray
  vtkIntArray* cycleArray =
    vtkIntArray::SafeDownCast(mgrid->GetFieldData()->GetArray("CycleNumber"));
  cycleArray->SetTuple1(0, *cycleNum);

  // Set FieldData for SimulationTime TimeArray
  vtkFloatArray* simTimeArray =
    vtkFloatArray::SafeDownCast(mgrid->GetFieldData()->GetArray("SimulationTime"));
  simTimeArray->SetTuple1(0, (float)*simTime);

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
// Add locations for all markers on all processors in the simulation
//
///////////////////////////////////////////////////////////////////////////////

void addmarkergeometry_(int* numberAdded, float* xloc, float* yloc, float* zloc)
{
  vtkMultiBlockDataSet* mgrid = vtkMultiBlockDataSet::SafeDownCast(
    vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("MarkerPointData")->GetGrid());
  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(mgrid->GetBlock(0));

  // Get pointers to points and cells which were allocated but not inserted
  vtkPoints* points = ugrid->GetPoints();

  for (int i = 0; i < *numberAdded; i++)
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

void addmarkerscalarfield_(char* fname, int* len, int* numberAdded, float* data)
{
  vtkMultiBlockDataSet* mgrid = vtkMultiBlockDataSet::SafeDownCast(
    vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("MarkerPointData")->GetGrid());
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
  for (int i = 0; i < *numberAdded; i++)
  {
    dataArray->InsertNextValue((float)data[i]);
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Set a vector field in the unstructured grid of markers
//
///////////////////////////////////////////////////////////////////////////////

void addmarkervectorfield_(
  char* fname, int* len, int* numberAdded, float* data0, float* data1, float* data2)
{
  vtkMultiBlockDataSet* mgrid = vtkMultiBlockDataSet::SafeDownCast(
    vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("MarkerPointData")->GetGrid());
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
  for (int i = 0; i < *numberAdded; i++)
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

void addmarkertensorfield_(char* fname, int* len, int* numberAdded, float* data0, float* data1,
  float* data2, float* data3, float* data4, float* data5)
{
  vtkMultiBlockDataSet* mgrid = vtkMultiBlockDataSet::SafeDownCast(
    vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("MarkerPointData")->GetGrid());
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
  for (int i = 0; i < *numberAdded; i++)
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
