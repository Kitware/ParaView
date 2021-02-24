/*=========================================================================

  Program:   ParaView
  Module:    vtkCPAdaptorAPI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCPAdaptorAPI.h"

#include "vtkCPDataDescription.h"
#include "vtkCPInputDataDescription.h"
#include "vtkCPProcessor.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataSet.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPointData.h"

#include <iostream>

// This code is meant as an API for Fortran and C simulation codes.
namespace ParaViewCoProcessing
{

/// Clear all of the field data from the grids.
void ClearFieldDataFromGrid(vtkDataSet* grid)
{
  if (grid)
  {
    grid->GetPointData()->Initialize();
    grid->GetCellData()->Initialize();
    // probably don't need to clear out field data but just being safe
    grid->GetFieldData()->Initialize();
  }
}
} // end namespace

vtkCPDataDescription* vtkCPAdaptorAPI::CoProcessorData = nullptr;
vtkCPProcessor* vtkCPAdaptorAPI::CoProcessor = nullptr;
bool vtkCPAdaptorAPI::IsTimeDataSet = false;

//-----------------------------------------------------------------------------
void vtkCPAdaptorAPI::CoProcessorInitialize()
{
  if (!vtkCPAdaptorAPI::CoProcessor)
  {
    vtkCPAdaptorAPI::CoProcessor = vtkCPProcessor::New();
    vtkCPAdaptorAPI::CoProcessor->Initialize();

    // no pipelines in this configuration
  }

  if (!vtkCPAdaptorAPI::CoProcessorData)
  {
    vtkCPAdaptorAPI::CoProcessorData = vtkCPDataDescription::New();
    vtkCPAdaptorAPI::CoProcessorData->AddInput("input");
  }
}

//-----------------------------------------------------------------------------
void vtkCPAdaptorAPI::CoProcessorFinalize()
{
  if (vtkCPAdaptorAPI::CoProcessor)
  {
    vtkCPAdaptorAPI::CoProcessor->Delete();
    vtkCPAdaptorAPI::CoProcessor = nullptr;
  }

  if (vtkCPAdaptorAPI::CoProcessorData)
  {
    vtkCPAdaptorAPI::CoProcessorData->Delete();
    vtkCPAdaptorAPI::CoProcessorData = nullptr;
  }
}

//-----------------------------------------------------------------------------
void vtkCPAdaptorAPI::RequestDataDescription(
  int* timeStep, double* time, int* coprocessThisTimeStep)
{
  if (!vtkCPAdaptorAPI::CoProcessorData || !vtkCPAdaptorAPI::CoProcessor)
  {
    vtkGenericWarningMacro("Problem in needtocoprocessthistimestep."
      << "Probably need to initialize.");
    *coprocessThisTimeStep = 0;
    return;
  }
  vtkIdType tStep = *timeStep;
  vtkCPAdaptorAPI::CoProcessorData->SetTimeData(*time, tStep);
  if (vtkCPAdaptorAPI::CoProcessor->RequestDataDescription(vtkCPAdaptorAPI::CoProcessorData))
  {
    *coprocessThisTimeStep = 1;
    vtkCPAdaptorAPI::IsTimeDataSet = true;
  }
  else
  {
    *coprocessThisTimeStep = 0;
    vtkCPAdaptorAPI::IsTimeDataSet = false;
  }
}

//-----------------------------------------------------------------------------
void vtkCPAdaptorAPI::NeedToCreateGrid(int* needGrid)
{
  if (!vtkCPAdaptorAPI::IsTimeDataSet)
  {
    vtkGenericWarningMacro("Time data not set.");
    *needGrid = 0;
    return;
  }

  // assume that the grid is not changing so that we only build it
  // the first time, otherwise we clear out the field data
  if (vtkCPAdaptorAPI::CoProcessorData->GetInputDescriptionByName("input")->GetGrid())
  {
    *needGrid = 0;
    // The grid is either stored as a class derived from vtkDataSet
    // or from a class derived from vtkMultiBlockDataSet
    if (vtkDataSet* grid = vtkDataSet::SafeDownCast(
          vtkCPAdaptorAPI::CoProcessorData->GetInputDescriptionByName("input")->GetGrid()))
    {
      ParaViewCoProcessing::ClearFieldDataFromGrid(grid);
    }
    else
    {
      vtkMultiBlockDataSet* multiBlock = vtkMultiBlockDataSet::SafeDownCast(
        vtkCPAdaptorAPI::CoProcessorData->GetInputDescriptionByName("input")->GetGrid());
      if (multiBlock)
      {
        vtkCompositeDataIterator* iter = multiBlock->NewIterator();
        iter->InitTraversal();
        for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
        {
          ParaViewCoProcessing::ClearFieldDataFromGrid(
            vtkDataSet::SafeDownCast(iter->GetCurrentDataObject()));
        }
        iter->Delete();
      }
    }
  }
  else
  {
    *needGrid = 1;
  }
}

//-----------------------------------------------------------------------------
void vtkCPAdaptorAPI::CoProcess()
{
  if (!vtkCPAdaptorAPI::IsTimeDataSet)
  {
    vtkGenericWarningMacro("Time data not set.");
  }
  else
  {
    vtkCPAdaptorAPI::CoProcessor->CoProcess(vtkCPAdaptorAPI::CoProcessorData);
  }
  // Reset time data.
  vtkCPAdaptorAPI::IsTimeDataSet = false;
}
