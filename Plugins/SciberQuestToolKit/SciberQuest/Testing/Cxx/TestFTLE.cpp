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
#include "vtkAppendPolyData.h"
#include "vtkProcessIdScalars.h"
#include "vtkSphereSource.h"
#include "vtkSQPlaneSource.h"
#include "vtkSQBOVMetaReader.h"
#include "vtkSQFieldTracer.h"
#include "vtkSQFTLE.h"
#include "vtkSQVolumeSource.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCamera.h"
#include "vtkPolyDataWriter.h"
#include "vtkTesting.h"
#include "vtkWindowToImageFilter.h"
#include "vtkPNGWriter.h"
#include "TestUtils.h"

#include <iostream>
#include <string>

int TestFTLE(int argc, char *argv[])
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
  inputFileName=dataRoot+"/SciberQuestToolKit/Gyres/Gyres.bov";

  std::string logFileName;
  logFileName=NativePath(tempDir+"/SciberQuestToolKit-TestFTLE.log");
  vtkSQLog::GetGlobalInstance()->SetFileName(logFileName.c_str());
  vtkSQLog::GetGlobalInstance()->SetGlobalLevel(1);

  // ooc reader
  vtkSQBOVMetaReader *r=vtkSQBOVMetaReader::New();
  r->SetFileName(inputFileName.c_str());
  r->SetPointArrayStatus("v",1);
  r->SetBlockSize(32,32,1);

  // seed points
  int ext[6];
  r->GetSubset(ext);

  vtkSQPlaneSource *sp=vtkSQPlaneSource::New();
  sp->SetOrigin(ext[0],ext[2],ext[4]);
  sp->SetPoint1(ext[1],ext[2],ext[4]);
  sp->SetPoint2(ext[0],ext[3],ext[4]);
  sp->SetXResolution(96);
  sp->SetYResolution(96);

  // displacement mapper
  vtkSQFieldTracer *dm=vtkSQFieldTracer::New();
  dm->SetMode(vtkSQFieldTracer::MODE_DISPLACEMENT);
  dm->SetIntegratorType(vtkSQFieldTracer::INTEGRATOR_RK4);
  dm->SetForwardOnly(0);
  dm->SetMaxStep(0.1);
  dm->SetNullThreshold(0.001);
  dm->SetMaxIntegrationInterval(48);
  dm->SetUseDynamicScheduler(1);
  dm->SetMasterBlockSize(8);
  dm->SetWorkerBlockSize(32);
  dm->AddInputConnection(0,r->GetOutputPort(0));
  dm->AddInputConnection(1,sp->GetOutputPort(0));
  dm->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,"v");

  r->Delete();
  sp->Delete();

  // ftle
  vtkSQFTLE *ftle=vtkSQFTLE::New();
  ftle->SetPassInput(1);
  ftle->AddInputArray("fwd-displacement-map");
  ftle->AddInputArray("bwd-displacement-map");
  ftle->AddInputArray("displacement");
  ftle->SetInputConnection(0,dm->GetOutputPort(0));

  dm->Delete();

  // process id
  vtkProcessIdScalars *pid=vtkProcessIdScalars::New();
  pid->SetInputConnection(0,ftle->GetOutputPort(0));
  ftle->Delete();

  // unstructured to polydata (needed for volumetric maps)
  vtkDataSetSurfaceFilter *surf=vtkDataSetSurfaceFilter::New();
  surf->SetInputConnection(0,pid->GetOutputPort(0));
  pid->Delete();

  // execute
  GetParallelExec(worldRank, worldSize, surf, 0.0);
  surf->Update();

  int testStatus = SerialRender(
        controller,
        surf->GetOutput(),
        false,
        tempDir,
        baseline,
        "SciberQuestToolKit-TestFTLE",
        400,400,
        63,63,128,
        63,63,0,
        0,1,0,
        1.25);

  surf->Delete();

  return Finalize(controller,testStatus==vtkTesting::PASSED?0:1);
}
