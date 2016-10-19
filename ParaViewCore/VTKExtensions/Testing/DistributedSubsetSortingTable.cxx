/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DistributedData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Test simple sorting on a distributed wavelet
// This test requires 4 MPI processes.

#include "vtkAttributeDataToTableFilter.h"
#include "vtkDistributedDataFilter.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkMPIController.h"
#include "vtkObjectFactory.h"
#include "vtkPieceScalars.h"
#include "vtkRTAnalyticSource.h"
#include "vtkSortedTableStreamer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
/*
** This test only builds if MPI is in use
*/
#include "vtkMPICommunicator.h"

#include "vtkProcess.h"

class MyProcess : public vtkProcess
{
public:
  static MyProcess* New();
  vtkTypeMacro(MyProcess, vtkProcess);

  virtual void Execute();

  void SetArgs(int anArgc, char* anArgv[]);

protected:
  MyProcess();

  int Argc;
  char** Argv;
};

vtkStandardNewMacro(MyProcess);

MyProcess::MyProcess()
{
  this->Argc = 0;
  this->Argv = 0;
}

void MyProcess::SetArgs(int anArgc, char* anArgv[])
{
  this->Argc = anArgc;
  this->Argv = anArgv;
}

void MyProcess::Execute()
{
  this->ReturnValue = 1;
  int me = this->Controller->GetLocalProcessId();
  int nbProc = this->Controller->GetNumberOfProcesses();

  // Dataset
  vtkRTAnalyticSource* wavelet = vtkRTAnalyticSource::New();
  // wavelet->SetWholeExtent(-50,50,-50,50,-50,50);

  // COLOR BY PROCESS NUMBER

  vtkPieceScalars* ps = vtkPieceScalars::New();
  ps->SetInputConnection(wavelet->GetOutputPort());
  ps->SetScalarModeToCellData();

  // MORE FILTERING - DS -> vtkTable

  vtkAttributeDataToTableFilter* dsToTableFilter = vtkAttributeDataToTableFilter::New();
  dsToTableFilter->SetInputConnection(ps->GetOutputPort());

  vtkSortedTableStreamer* sortFilter = vtkSortedTableStreamer::New();
  sortFilter->SetInputConnection(dsToTableFilter->GetOutputPort());
  sortFilter->SetColumnNameToSort("RTData");
  sortFilter->SetSelectedComponent(0);
  sortFilter->SetBlock(0);
  sortFilter->SetBlockSize(1024);
  sortFilter->UpdatePiece(me, nbProc, 0);

  //  cout << "Full range ["
  //       << wavelet->GetOutput()->GetScalarRange()[0]
  //       << ", "
  //       << wavelet->GetOutput()->GetScalarRange()[1]
  //       << "]"
  //       << endl;

  if (me == 0)
  {
    vtkFloatArray* data =
      vtkFloatArray::SafeDownCast(sortFilter->GetOutput()->GetColumnByName("RTData"));
    //      cout << ">>> Print block " << i << " range: [" << data->GetRange()[0]
    //           << ", " << data->GetRange()[1] << "] - size: "
    //           << data->GetNumberOfTuples() << endl;
    double goal = data->GetRange()[0];
    vtkIdType index = -1;
    while (goal == data->GetValue(++index))
      ;
    cout << "the first " << index << " values are the same. The nb proc is " << nbProc << endl;
    this->ReturnValue = ((index % nbProc) == 0);
    if (this->ReturnValue == 1)
    {
      cout << "First block values are the same. OK" << endl;
    }
  }

  // CLEAN UP
  wavelet->Delete();
  ps->Delete();
  sortFilter->Delete();
  dsToTableFilter->Delete();
}

int main(int argc, char** argv)
{
  int retVal = 1;

  vtkMPIController* contr = vtkMPIController::New();
  contr->Initialize(&argc, &argv);

  vtkMultiProcessController::SetGlobalController(contr);

  int numProcs = contr->GetNumberOfProcesses();
  int me = contr->GetLocalProcessId();

  if (me == 0 && !vtkSortedTableStreamer::TestInternalClasses())
  {
    contr->Delete();
    return retVal;
  }

  if (numProcs < 2)
  {
    if (me == 0)
    {
      cout << "DistributedData test requires more than 1 processe" << endl;
    }
    contr->Delete();
    return retVal;
  }

  if (!contr->IsA("vtkMPIController"))
  {
    if (me == 0)
    {
      cout << "DistributedData test requires MPI" << endl;
    }
    contr->Delete();
    return retVal; // is this the right error val?   TODO
  }

  MyProcess* p = MyProcess::New();
  p->SetArgs(argc, argv);
  contr->SetSingleProcessObject(p);
  contr->SingleMethodExecute();

  retVal = p->GetReturnValue();
  p->Delete();

  contr->Finalize();
  contr->Delete();

  return !retVal;
}
