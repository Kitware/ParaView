/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQLog.h"
#include "vtkMultiProcessController.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkAppendPolyData.h"
#include "vtkProcessIdScalars.h"
#include "vtkSphereSource.h"
#include "vtkSQPlaneSource.h"
#include "vtkSQBOVMetaReader.h"
#include "vtkSQFieldTracer.h"
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

int TestFieldTopologyMapper(int argc, char *argv[])
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
  inputFileName=NativePath(dataRoot+"/SciberQuestToolKit/SmallVector/SmallVector.bovm");

  std::string logFileName;
  logFileName=NativePath(tempDir+"/SciberQuestToolKit-TestFieldTopologyMapper.log");
  vtkSQLog::GetGlobalInstance()->SetFileName(logFileName.c_str());
  vtkSQLog::GetGlobalInstance()->SetGlobalLevel(1);

  // TODO -- make this a command line option
  enum {
    PLANE=1,
    VOLUME=2
    };
  int seedSelection=PLANE;

  // ooc reader
  vtkSQBOVMetaReader *r=vtkSQBOVMetaReader::New();
  r->SetFileName(inputFileName.c_str());
  r->SetPointArrayStatus("vi",1);
  r->SetBlockSize(8,8,8);
  r->SetBlockCacheSize(1);

  // terminator
  vtkSphereSource *s1=vtkSphereSource::New();
  s1->SetCenter(3.5,6.0,1.5);
  s1->SetRadius(1.0);
  s1->SetThetaResolution(32);
  s1->SetPhiResolution(32);

  vtkSphereSource *s2=vtkSphereSource::New();
  s2->SetCenter(3.5,5.0,2.0);
  s2->SetRadius(0.5);
  s2->SetThetaResolution(32);
  s2->SetPhiResolution(32);

  vtkSphereSource *s3=vtkSphereSource::New();
  s3->SetCenter(3.5,1.0,6.0);
  s3->SetRadius(0.8);
  s3->SetThetaResolution(32);
  s3->SetPhiResolution(32);

  vtkSphereSource *s4=vtkSphereSource::New();
  s4->SetCenter(3.5,1.45,5.1);
  s4->SetRadius(0.25);
  s4->SetThetaResolution(32);
  s4->SetPhiResolution(32);

  // seed points
  vtkAlgorithm *sp=0;
  switch (seedSelection)
    {
    case PLANE:
      {
      vtkSQPlaneSource *p=vtkSQPlaneSource::New();
      p->SetOrigin(1.0,3.5,0.25);
      p->SetPoint1(6.0,3.5,0.25);
      p->SetPoint2(1.0,3.5,4.75);
      p->SetXResolution(100);
      p->SetYResolution(100);
      sp=p;
      }
      break;

    case VOLUME:
      {
      vtkSQVolumeSource *v=vtkSQVolumeSource::New();
      v->SetOrigin(1.0,3.0,0.25);
      v->SetPoint1(6.0,3.0,0.25);
      v->SetPoint2(1.0,4.0,0.25);
      v->SetPoint3(1.0,3.0,4.75);
      v->SetResolution(60,12,60);
      sp=v;
      }
      break;

    default:
      std::cerr << "Error: invalid seed point selection " << seedSelection << std::endl;
      return 1;
      break;
    }

  // field topology mapper
  vtkSQFieldTracer *ftm=vtkSQFieldTracer::New();
  ftm->SetMode(vtkSQFieldTracer::MODE_TOPOLOGY);
  ftm->SetIntegratorType(vtkSQFieldTracer::INTEGRATOR_RK45);
  ftm->SetMinStep(1.0e-8);
  ftm->SetMaxStep(0.1);
  ftm->SetMaxError(0.001);
  ftm->SetMaxNumberOfSteps(10000);
  ftm->SetMaxLineLength(70);
  ftm->SetNullThreshold(0.001);
  ftm->SetSqueezeColorMap(1);
  ftm->SetForwardOnly(0);
  ftm->SetUseDynamicScheduler(1);
  ftm->SetMasterBlockSize(0);
  ftm->SetWorkerBlockSize(128);
  ftm->AddInputConnection(0,r->GetOutputPort(0));
  ftm->AddInputConnection(1,sp->GetOutputPort(0));
  ftm->AddInputConnection(2,s1->GetOutputPort(0));
  ftm->AddInputConnection(2,s2->GetOutputPort(0));
  ftm->AddInputConnection(2,s3->GetOutputPort(0));
  ftm->AddInputConnection(2,s4->GetOutputPort(0));
  ftm->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,"vi");

  r->Delete();
  sp->Delete();
  s1->Delete();
  s2->Delete();
  s3->Delete();
  s4->Delete();

  // process id
  vtkProcessIdScalars *pid=vtkProcessIdScalars::New();
  pid->SetInputConnection(0,ftm->GetOutputPort(0));
  ftm->Delete();

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
        "SciberQuestToolKit-TestFieldTopologyMapper",
        400,340,
        0,15,3.5,
        0,3.5,3.5,
        0,0,1,
        1.45);

  surf->Delete();

  return Finalize(controller,testStatus==vtkTesting::PASSED?0:1);
}
