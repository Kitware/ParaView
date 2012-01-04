/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile FortranAdaptorAPI.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "FortranAdaptorAPI.h"

#include <iostream>
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCPDataDescription.h"
#include "vtkCPInputDataDescription.h"
#include "vtkCPProcessor.h"
#include "vtkCPPythonScriptPipeline.h"
#include "vtkDataSet.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPointData.h"

// This code is meant as an API for Fortran and C simulation codes.

namespace ParaViewCoProcessing
{
  vtkCPProcessor* coProcessor = 0;
  vtkCPDataDescription* coProcessorData = 0;
  // IsTimeDataSet is meant to be used to make sure that
  // needtocoprocessthistimestep() is called before
  // calling any of the other coprocessing functions.
  // It is reset to falase after calling coprocess as well
  // as if coprocessing is not needed for this time/time step
  bool isTimeDataSet = 0;

  vtkCPDataDescription* GetCoProcessorData()
  {
    return coProcessorData;
  }

  void ClearFieldDataFromGrid(vtkDataSet* grid)
  {
    if(grid)
      {
      grid->GetPointData()->Initialize();
      grid->GetCellData()->Initialize();
      // probably don't need to clear out field data but just being safe
      grid->GetFieldData()->Initialize();
      }
  }

  // returns true if successful, false otherwise (e.g. if
  // CStringMaxLength <= FortranStringLength)
  bool ConvertFortranStringToCString(char* fortranString, int fortranStringLength,
                                     char* cString, int cStringMaxLength)
  {
    if(fortranStringLength >= cStringMaxLength)
      {
      return false;
      }

    for(int i=0;i<fortranStringLength;i++)
      {
      cString[i] = fortranString[i];
      }
    cString[fortranStringLength] = '\0';
    return true;
  }
} // end namespace

void coprocessorinitialize(char* pythonFileName, int* pythonFileNameLength )
{
  if(!ParaViewCoProcessing::coProcessor)
    {
    char cPythonFileName[200];
    ParaViewCoProcessing::ConvertFortranStringToCString(
      pythonFileName, *pythonFileNameLength, cPythonFileName, 200);

    vtkCPPythonScriptPipeline* pipeline = vtkCPPythonScriptPipeline::New();
    pipeline->Initialize(cPythonFileName);

    ParaViewCoProcessing::coProcessor = vtkCPProcessor::New();
    ParaViewCoProcessing::coProcessor->Initialize();
    ParaViewCoProcessing::coProcessor->AddPipeline(pipeline);
    pipeline->Delete();
    }
  if(!ParaViewCoProcessing::coProcessorData)
    {
    ParaViewCoProcessing::coProcessorData = vtkCPDataDescription::New();
    ParaViewCoProcessing::coProcessorData->AddInput("input");
    }
}

void coprocessorfinalize()
{
  if(ParaViewCoProcessing::coProcessor)
    {
    ParaViewCoProcessing::coProcessor->Delete();
    ParaViewCoProcessing::coProcessor = 0;
    }
  if(ParaViewCoProcessing::coProcessorData)
    {
    ParaViewCoProcessing::coProcessorData->Delete();
    ParaViewCoProcessing::coProcessorData = 0;
    }
}

void requestdatadescription(int* timeStep, double* time,
                            int* coprocessThisTimeStep)
{
  if(!ParaViewCoProcessing::coProcessorData || !ParaViewCoProcessing::coProcessor)
    {
    vtkGenericWarningMacro("Problem in needtocoprocessthistimestep."
                           << "Probably need to initialize.");
    *coprocessThisTimeStep = 0;
    return;
    }
  vtkIdType tStep = *timeStep;
  ParaViewCoProcessing::coProcessorData->SetTimeData(*time, tStep);
  if(ParaViewCoProcessing::coProcessor->RequestDataDescription(
       ParaViewCoProcessing::coProcessorData))
    {
    *coprocessThisTimeStep = 1;
    ParaViewCoProcessing::isTimeDataSet = true;
    }
  else
    {
    *coprocessThisTimeStep = 0;
    ParaViewCoProcessing::isTimeDataSet = false;
    }
}

void needtocreategrid(int* needGrid)
{
  if(!ParaViewCoProcessing::isTimeDataSet)
    {
    vtkGenericWarningMacro("Time data not set.");
    *needGrid = 0;
    return;
    }

  // assume that the grid is not changing so that we only build it
  // the first time, otherwise we clear out the field data
  if(ParaViewCoProcessing::coProcessorData->GetInputDescriptionByName("input")->GetGrid())
    {
    *needGrid = 0;
    // The grid is either stored as a class derived from vtkDataSet
    // or from a class derived from vtkMultiBlockDataSet
    if(vtkDataSet* grid = vtkDataSet::SafeDownCast(
         ParaViewCoProcessing::coProcessorData->GetInputDescriptionByName("input")->GetGrid()))
      {
      ParaViewCoProcessing::ClearFieldDataFromGrid(grid);
      }
    else
      {
      vtkMultiBlockDataSet* multiBlock = vtkMultiBlockDataSet::SafeDownCast(
        ParaViewCoProcessing::coProcessorData->GetInputDescriptionByName("input")->GetGrid());
      if(multiBlock)
        {
        vtkCompositeDataIterator* iter = multiBlock->NewIterator();
        iter->VisitOnlyLeavesOn();
        iter->TraverseSubTreeOn();
        iter->SkipEmptyNodesOn();
        iter->InitTraversal();
        for(iter->GoToFirstItem();!iter->IsDoneWithTraversal();iter->GoToNextItem())
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

void coprocess()
{
  if(!ParaViewCoProcessing::isTimeDataSet)
    {
    vtkGenericWarningMacro("Time data not set.");
    }
  else
    {
    ParaViewCoProcessing::coProcessor->CoProcess(ParaViewCoProcessing::coProcessorData);
    }
  // Reset time data.
  ParaViewCoProcessing::isTimeDataSet = false;
}
