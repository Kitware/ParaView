/*=========================================================================

  Program:   ParaView
  Module:    fv_create_data.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "fv_create_data.h"

#include "Grid.h"

#include "vtkCPDataDescription.h"
#include "vtkCPInputDataDescription.h"
#include "vtkCPProcessor.h"
#include "vtkCPPythonScriptPipeline.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

#include <sstream>

namespace
{
vtkCPProcessor* g_coprocessor;                     /// catalyst coprocessor
vtkCPDataDescription* g_coprocessorData;           /// input, sinput, input3D, sinput3D
bool g_isTimeDataSet;                              /// is time data set?
CamAdaptor::Grid<CamAdaptor::RECTILINEAR>* g_grid; /// rectilinear grid (2D, 3D)
CamAdaptor::Grid<CamAdaptor::SPHERE>* g_sgrid;     /// structured (spherical) (2D, 3Da) grids

//------------------------------------------------------------------------------
/// Deletes global data
void fv_finalize()
{
  if (g_grid)
  {
    delete g_grid;
  }
  if (g_sgrid)
  {
    delete g_sgrid;
  }
}

//------------------------------------------------------------------------------
/// Deletes the Catalyt Coprocessor and data
void fv_coprocessorfinalize()
{
  if (g_coprocessor)
  {
    g_coprocessor->Delete();
    g_coprocessor = NULL;
  }
  if (g_coprocessorData)
  {
    g_coprocessorData->Delete();
    g_coprocessorData = NULL;
  }
}
} // anonymous namespace

//------------------------------------------------------------------------------
void fv_coprocessorinitializewithpython_(const char* pythonScriptName)
{
  if (!g_coprocessor)
  {
    g_coprocessor = vtkCPProcessor::New();
    g_coprocessor->Initialize();
    // python pipeline
    vtkSmartPointer<vtkCPPythonScriptPipeline> pipeline =
      vtkSmartPointer<vtkCPPythonScriptPipeline>::New();
    pipeline->Initialize(pythonScriptName);
    g_coprocessor->AddPipeline(pipeline);
  }
  if (!g_coprocessorData)
  {
    g_coprocessorData = vtkCPDataDescription::New();
    g_coprocessorData->AddInput("input");
    g_coprocessorData->AddInput("input3D");
    g_coprocessorData->AddInput("sinput");
    g_coprocessorData->AddInput("sinput3D");
  }
}

//------------------------------------------------------------------------------
void fv_create_grid_(int* dim, double* lonCoord, double* latCoord, double* levCoord, int* nCells2d,
  int* maxNcols, int* myRank)
{
  // printCreateGrid(dim, lonCoord, latCoord, levCoord, nCells2d, maxNcols, myRank);
  if (!g_coprocessorData)
  {
    vtkGenericWarningMacro("Unable to access CoProcessorData.");
    return;
  }
  g_grid = new CamAdaptor::Grid<CamAdaptor::RECTILINEAR>();
  g_grid->SetMpiRank(*myRank);
  g_grid->SetChunkCapacity(*maxNcols);
  g_grid->SetNCells2d(*nCells2d);
  g_grid->SetNLon(dim[0]);
  g_grid->SetNLat(dim[1]);
  g_grid->SetLev(dim[2], levCoord);
  g_grid->SetLonStep(lonCoord[1] - lonCoord[0]);
  g_grid->SetLatStep(latCoord[1] - latCoord[0]);
  g_grid->Create();
  if (!CamAdaptor::Grid<CamAdaptor::RECTILINEAR>::SetToCoprocessor(
        g_coprocessorData, "input", g_grid->GetGrid2d()) ||
    !CamAdaptor::Grid<CamAdaptor::RECTILINEAR>::SetToCoprocessor(
      g_coprocessorData, "input3D", g_grid->GetGrid3d()))
  {
    vtkGenericWarningMacro(<< "No input data description");
    delete g_grid;
    g_grid = NULL;
  }

  g_sgrid = new CamAdaptor::Grid<CamAdaptor::SPHERE>();
  g_sgrid->SetMpiRank(*myRank);
  g_sgrid->SetChunkCapacity(*maxNcols);
  g_sgrid->SetNCells2d(*nCells2d);
  g_sgrid->SetNLon(dim[0]);
  g_sgrid->SetNLat(dim[1]);
  g_sgrid->SetLev(dim[2], levCoord);
  g_sgrid->SetLonStep(lonCoord[1] - lonCoord[0]);
  g_sgrid->SetLatStep(latCoord[1] - latCoord[0]);
  g_sgrid->Create();
  if (!CamAdaptor::Grid<CamAdaptor::SPHERE>::SetToCoprocessor(
        g_coprocessorData, "sinput", g_sgrid->GetGrid2d()) ||
    !CamAdaptor::Grid<CamAdaptor::SPHERE>::SetToCoprocessor(
      g_coprocessorData, "sinput3D", g_sgrid->GetGrid3d()))
  {
    vtkGenericWarningMacro(<< "No input data description");
    delete g_sgrid;
    g_sgrid = NULL;
  }
}

//------------------------------------------------------------------------------
void fv_add_chunk_(int* nstep, int* chunkSize, double* lonRad, double* latRad, double* psScalar,
  double* tScalar, double* uScalar, double* vScalar)
{
  if (*nstep == 0)
  {
    for (int i = 0; i < *chunkSize; ++i)
    {
      if (g_grid)
      {
        g_grid->AddPointsAndCells(lonRad[i], latRad[i]);
      }
      if (g_sgrid)
      {
        g_sgrid->AddPointsAndCells(lonRad[i], latRad[i]);
      }
    }
  }
  if (g_grid)
  {
    // g_grid->PrintAddChunk(
    //   nstep, chunkSize, lonRad, latRad, psScalar, tScalar);
    g_grid->SetAttributeValue(*chunkSize, lonRad, latRad, psScalar, tScalar, uScalar, vScalar);
  }
  if (g_sgrid)
  {
    g_sgrid->SetAttributeValue(*chunkSize, lonRad, latRad, psScalar, tScalar, uScalar, vScalar);
  }
}

//------------------------------------------------------------------------------
void fv_catalyst_finalize_()
{
  fv_finalize();
  fv_coprocessorfinalize();
}

//------------------------------------------------------------------------------
int fv_requestdatadescription_(int* timeStep, double* time)
{
  if (!g_coprocessorData || !g_coprocessor)
  {
    vtkGenericWarningMacro("Data or coprocessor are not initialized.");
    return 0;
  }
  vtkIdType tStep = *timeStep;
  g_coprocessorData->SetTimeData(*time, tStep);
  if (g_coprocessor->RequestDataDescription(g_coprocessorData))
  {
    g_isTimeDataSet = true;
    return 1;
  }
  else
  {
    g_isTimeDataSet = false;
    return 0;
  }
}

//------------------------------------------------------------------------------
int fv_needtocreategrid_()
{
  if (!g_isTimeDataSet)
  {
    vtkGenericWarningMacro("Time data not set.");
    return 0;
  }

  // assume that the grid is not changing so that we only build it
  // the first time, otherwise we clear out the field data
  vtkCPInputDataDescription* idd = g_coprocessorData->GetInputDescriptionByName("input");
  return (idd == NULL || idd->GetGrid() == NULL);
}

//------------------------------------------------------------------------------
void fv_coprocess_()
{
  if (!g_isTimeDataSet)
  {
    vtkGenericWarningMacro("Time data not set.");
  }
  else
  {
    g_coprocessor->CoProcess(g_coprocessorData);
  }
  // Reset time data.
  g_isTimeDataSet = false;
}
