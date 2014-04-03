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
#include "vtkProcessIdScalars.h"
#include "vtkPlaneSource.h"
#include "vtkSQBOVMetaReader.h"
#include "vtkSQBOVReader.h"
#include "vtkSQFieldTracer.h"
#include "vtkSQVolumeSource.h"
#include "vtkSQPlaneSource.h"
#include "vtkSQLineSource.h"
#include "vtkSQPointSource.h"
#include "vtkSQTubeFilter.h"
#include "vtkAppendPolyData.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPolyData.h"
#include "vtkTesting.h"
#include "TestUtils.h"

#include <iostream>
#include <string>

int TestFieldTracer(int argc, char *argv[])
{
  vtkMultiProcessController *controller=Initialize(&argc,&argv);
  int worldRank=controller->GetLocalProcessId();
  int worldSize=controller->GetNumberOfProcesses();

  // configure
  std::string dataRoot;
  std::string tempDir;
  std::string baseline;
  BroadcastConfiguration(controller,argc,argv,dataRoot,tempDir,baseline);

  std::string inputFileName;
  inputFileName=dataRoot+"/SciberQuestToolKit/MagneticIslands/MagneticIslands.bov";

  std::string logFileName;
  logFileName=NativePath(tempDir+"/SciberQuestToolKit-TestFieldTracer.log");
  vtkSQLog::GetGlobalInstance()->SetFileName(logFileName.c_str());
  vtkSQLog::GetGlobalInstance()->SetGlobalLevel(1);

  // pipeline 1
  // ooc reader
  vtkSQBOVMetaReader *mr=vtkSQBOVMetaReader::New();
  mr->SetFileName(inputFileName.c_str());
  mr->SetPointArrayStatus("b",1);
  mr->SetNumberOfGhostCells(2);
  mr->SetXHasPeriodicBC(1);
  mr->SetYHasPeriodicBC(1);
  mr->SetZHasPeriodicBC(1);
  mr->SetBlockSize(8,8,8);
  mr->SetBlockCacheSize(1);

  // seed points
  vtkSQLineSource *p1=vtkSQLineSource::New();
  p1->SetPoint1(-0.125,-0.125,0.0);
  p1->SetPoint2(-0.5,-0.5,0.0);
  p1->SetResolution(3);

  // field tracer
  vtkSQFieldTracer *ft=vtkSQFieldTracer::New();
  ft->SetMode(vtkSQFieldTracer::MODE_STREAM);
  ft->SetIntegratorType(vtkSQFieldTracer::INTEGRATOR_RK4);
  ft->SetMaxStep(0.01);
  ft->SetMaxLineLength(300);
  //ft->SetMinSegmentLength(0.2); TODO --- this feature has a bug.
  ft->SetNullThreshold(0.001);
  ft->SetForwardOnly(0);
  ft->SetUseDynamicScheduler(0);
  ft->AddInputConnection(0,mr->GetOutputPort(0));
  ft->AddInputConnection(1,p1->GetOutputPort(0));
  ft->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,"b");
  mr->Delete();
  p1->Delete();

  // pids
  vtkProcessIdScalars *pid=vtkProcessIdScalars::New();
  pid->SetInputConnection(0,ft->GetOutputPort(0));
  ft->Delete();

  // tubes
  vtkSQTubeFilter *tf=vtkSQTubeFilter::New();
  tf->SetRadius(0.025);
  tf->SetNumberOfSides(16);
  tf->SetInputConnection(pid->GetOutputPort(0));
  pid->Delete();

  // execute
  GetParallelExec(worldRank,worldSize,tf,0.0);
  tf->Update();

  int testStatus = SerialRender(
        controller,
        (vtkPolyData*)tf->GetOutput(),
        true,
        tempDir,
        baseline,
        "SciberQuestToolKit-TestFieldTracer",
        600,600,
        13,13,13,
        0,0,0,
        0,0,1,
        1.1,
        30.0);

  tf->Delete();

  return Finalize(controller,testStatus==vtkTesting::PASSED?0:1);
}
