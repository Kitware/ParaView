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
#include "vtkCellType.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCPDataDescription.h"
#include "vtkCPInputDataDescription.h"
#include "vtkCPProcessor.h"
#include "vtkCPPythonScriptPipeline.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"

using namespace std;

// This code is meant as an API for fortran simulation codes.

namespace 
{
  vtkCPProcessor* coProcessor = 0;
  vtkCPDataDescription* coProcessorData = 0;
  // IsTimeDataSet is meant to be used to make sure that
  // needtocoprocessthistimestep() is called before 
  // calling any of the other coprocessing functions.
  // It is reset to falase after calling coprocess as well
  // as if coprocessing is not needed for this time/time step
  bool isTimeDataSet = 0;
}

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

void coprocessorinitialize_(char* pythonFileName, int* pythonFileNameLength )
{
  if(!coProcessor)
    {
    char cPythonFileName[200];
    ConvertFortranStringToCString(pythonFileName, *pythonFileNameLength, 
                                  cPythonFileName, 200);

    vtkCPPythonScriptPipeline* pipeline = vtkCPPythonScriptPipeline::New();
    pipeline->Initialize(cPythonFileName);

    coProcessor = vtkCPProcessor::New();
    coProcessor->Initialize();
    coProcessor->AddPipeline(pipeline);
    pipeline->Delete();
    }
  if(!coProcessorData)
    {
    coProcessorData = vtkCPDataDescription::New();
    coProcessorData->AddInput("input");
    }
}

void coprocessorfinalize_()
{
  if(coProcessor)
    {
    coProcessor->Delete();
    coProcessor = 0;
    }
  if(coProcessorData)
    {
    coProcessorData->Delete();
    coProcessorData = 0;
    }
}

void requestdatadescription_(int* timeStep, double* time, 
                             int* coprocessThisTimeStep)
{
  if(!coProcessorData || !coProcessor)
    {
    vtkGenericWarningMacro(
      "Problem in needtocoprocessthistimestep - probably need to initialize.");
    *coprocessThisTimeStep = 0;
    return;
    }
  vtkIdType tStep = *timeStep;
  coProcessorData->SetTimeData(*time, tStep);
  if(coProcessor->RequestDataDescription(coProcessorData))
    {
    *coprocessThisTimeStep = 1;
    isTimeDataSet = true;
    }
  else
    {
    *coprocessThisTimeStep = 0;
    isTimeDataSet = false;
    }
}

void needtocreategrid_(int* needGrid)
{
  if(!isTimeDataSet)
    {
    cout << "Time data not set.\n";
    *needGrid = 0;
    return;
    }

  // assume that the grid is not changing so that we only build it 
  // the first time, otherwise we clear out the field data
  if(coProcessorData->GetInputDescriptionByName("input")->GetGrid())
    {
    *needGrid = 0;
    // The grid is either stored as a class derived from vtkDataSet
    // or from a class derived from vtkMultiBlockDataSet
    vtkDataSet* grid = vtkDataSet::SafeDownCast(
      coProcessorData->GetInputDescriptionByName("input")->GetGrid());
    if(grid)
      {
      ClearFieldDataFromGrid(grid);
      }
    else
      {
      vtkMultiBlockDataSet* multiBlock = vtkMultiBlockDataSet::SafeDownCast(
        coProcessorData->GetInputDescriptionByName("input")->GetGrid());
      if(multiBlock)
        {
        vtkCompositeDataIterator* iter = multiBlock->NewIterator();
        iter->VisitOnlyLeavesOn();
        iter->TraverseSubTreeOn();
        iter->SkipEmptyNodesOn();
        iter->InitTraversal();
        for(iter->GoToFirstItem();!iter->IsDoneWithTraversal();iter->GoToNextItem())
          {
          ClearFieldDataFromGrid(vtkDataSet::SafeDownCast(iter->GetCurrentDataObject()));
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

void coprocess_()
{
  if(!isTimeDataSet)
    {
    cout << "Time data not set.\n";
    }
  else
    {
    coProcessor->CoProcess(coProcessorData);
    }
  // Reset time data.
  isTimeDataSet = false;
}
