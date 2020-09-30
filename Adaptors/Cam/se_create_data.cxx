/*=========================================================================

  Program:   ParaView
  Module:    se_create_data.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "se_create_data.h"

#include "Grid.h"

#include "vtkCPDataDescription.h"
#include "vtkCPInputDataDescription.h"
#include "vtkCPProcessor.h"
#include "vtkCPPythonScriptPipeline.h"
#include "vtkMath.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

#include <cfloat>
#include <iterator>
#include <set>
#include <sstream>

namespace
{
vtkCPProcessor* g_coprocessor;                     /// catalyst coprocessor
vtkCPDataDescription* g_coprocessorData;           /// sinput, sinput3d
bool g_isTimeDataSet;                              /// is time data set?
CamAdaptor::Grid<CamAdaptor::CUBE_SPHERE>* g_grid; /// 2d,3d cubed-spheres

//------------------------------------------------------------------------------
/// Deletes global data
void se_finalize()
{
  if (g_grid)
  {
    delete g_grid;
  }
}

//------------------------------------------------------------------------------
/// Deletes the Catalyt Coprocessor and data
void se_coprocessorfinalize()
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
void se_coprocessorinitializewithpython_(const char* pythonScriptName)
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
  }
}

//------------------------------------------------------------------------------
void se_create_grid_(int* ne, int* np, int* nlon, double* lonRad, int* nlat, double* latRad,
  int* nlev, double* lev, int* nCells2d, int* maxNcols, int* mpiRank)
{
  (void)nCells2d;
  if (!g_coprocessorData)
  {
    vtkGenericWarningMacro("Unable to access CoProcessorData.");
    return;
  }

  g_grid = new CamAdaptor::Grid<CamAdaptor::CUBE_SPHERE>();
  g_grid->SetMpiRank(*mpiRank);
  g_grid->SetChunkCapacity(*maxNcols);
  int points = *ne * *np + 1;
  g_grid->SetNCells2d(points * points * 6);
  g_grid->SetCubeGridPoints(*ne, *np, *nlon, lonRad, *nlat, latRad);
  g_grid->SetLev(*nlev, lev);
  g_grid->Create();
  if (!CamAdaptor::Grid<CamAdaptor::CUBE_SPHERE>::SetToCoprocessor(
        g_coprocessorData, "input", g_grid->GetGrid2d()) ||
    !CamAdaptor::Grid<CamAdaptor::CUBE_SPHERE>::SetToCoprocessor(
      g_coprocessorData, "input3D", g_grid->GetGrid3d()))
  {
    vtkGenericWarningMacro(<< "No input data description");
    delete g_grid;
    g_grid = NULL;
  }
}

//------------------------------------------------------------------------------
void se_add_chunk_(int* nstep, int* chunkSize, double* lonRad, double* latRad, double* psScalar,
  double* tScalar, double* uScalar, double* vScalar)
{
  if (*nstep == 0)
  {
    std::ostringstream ostr;
    ostr << "se_add_chunk: " << *chunkSize << std::endl;
    std::cerr << ostr.str();
    for (int i = 0; i < *chunkSize; ++i)
    {
      if (g_grid)
      {
        g_grid->AddPointsAndCells(lonRad[i], latRad[i]);
      }
    }
  }
  if (g_grid)
  {
    g_grid->SetAttributeValue(*chunkSize, lonRad, latRad, psScalar, tScalar, uScalar, vScalar);
  }
}

//------------------------------------------------------------------------------
int se_requestdatadescription_(int* timeStep, double* time)
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
int se_needtocreategrid_()
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
void se_coprocess_()
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

//------------------------------------------------------------------------------
void se_catalyst_finalize_()
{
  se_finalize();
  se_coprocessorfinalize();
}
