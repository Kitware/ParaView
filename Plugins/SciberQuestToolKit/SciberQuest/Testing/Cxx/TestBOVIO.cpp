/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkMultiProcessController.h"
#include "vtkSQLog.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkAppendPolyData.h"
#include "vtkProcessIdScalars.h"
#include "vtkSQBOVReader.h"
#include "vtkSQBOVWriter.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPolyData.h"
#include "vtkTesting.h"
#include "TestUtils.h"
#include <iostream>
#include <string>

int TestBOVIO(int argc, char *argv[])
{
  int aTestFailed=0;
  int result=vtkTesting::FAILED;

  vtkMultiProcessController *controller=Initialize(&argc,&argv);
  int worldRank=controller->GetLocalProcessId();
  int worldSize=controller->GetNumberOfProcesses();

  // configure
  std::string dataRoot;
  std::string tempDir;
  std::string baseline;
  BroadcastConfiguration(controller,argc,argv,dataRoot,tempDir,baseline);

  std::string inputFileName;
  inputFileName=NativePath(dataRoot+"/SciberQuestToolKit/Asym2D/Asym2D.bov");

  std::string tempOutputFileName;
  tempOutputFileName=NativePath(tempDir+"/SciberQuestToolKit-Asym2D.bov");

  std::string logFileName;
  logFileName=NativePath(tempDir+"/SciberQuestToolKit-TestBOVIO.log");
  vtkSQLog::GetGlobalInstance()->SetFileName(logFileName.c_str());
  vtkSQLog::GetGlobalInstance()->SetGlobalLevel(1);

  // reader
  vtkSQBOVReader *r1=vtkSQBOVReader::New();
  r1->SetFileName(inputFileName.c_str());
  r1->SetPointArrayStatus("ue",1);

  // process id
  vtkProcessIdScalars *p1=vtkProcessIdScalars::New();
  p1->SetInputConnection(0,r1->GetOutputPort(0));
  r1->Delete();

  // image to polydata
  vtkDataSetSurfaceFilter *s1=vtkDataSetSurfaceFilter::New();
  s1->SetInputConnection(0,p1->GetOutputPort(0));
  p1->Delete();

  // execute
  GetParallelExec(worldRank, worldSize, s1, 0.0);
  s1->Update();

  // render
  result = SerialRender(
        controller,
        s1->GetOutput(),
        false,
        tempDir,
        baseline,
        "SciberQuestToolKit-TestBOVIO",
        700,300,
        0,1,0,
        0,0,0,
        0,0,1,
        2.25);
  if (result==vtkTesting::FAILED)
    {
    aTestFailed=1;
    }

  // write
  vtkSQBOVWriter *bw=vtkSQBOVWriter::New();
  bw->SetInputConnection(r1->GetOutputPort(0));
  bw->SetFileName(tempOutputFileName.c_str());
  bw->Update();
  bw->Write();
  bw->Delete();
  s1->Delete();

  // read
  vtkSQBOVReader *r2=vtkSQBOVReader::New();
  r2->SetFileName(tempOutputFileName.c_str());
  r2->SetPointArrayStatus("ue",1);
  r2->SetPointArrayStatus("uex",1);
  r2->SetPointArrayStatus("uey",1);
  r2->SetPointArrayStatus("uez",1);

  vtkProcessIdScalars *p2=vtkProcessIdScalars::New();
  p2->SetInputConnection(0,r2->GetOutputPort(0));
  r2->Delete();

  vtkDataSetSurfaceFilter *s2=vtkDataSetSurfaceFilter::New();
  s2->SetInputConnection(0,p2->GetOutputPort(0));
  p2->Delete();

  // execute
  GetParallelExec(worldRank, worldSize, s2, 0.0);
  s2->Update();

  // render
  result = SerialRender(
        controller,
        s2->GetOutput(),
        false,
        tempDir,
        baseline,
        "SciberQuestToolKit-TestBOVIO",
        700,300,
        0,1,0,
        0,0,0,
        0,0,1,
        2.25);
  if (result==vtkTesting::FAILED)
    {
    aTestFailed=1;
    }
  s2->Delete();

  return Finalize(controller,aTestFailed);
}
