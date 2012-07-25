/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkMultiProcessController.h"
#include "vtkMPIController.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkAppendPolyData.h"
#include "vtkProcessIdScalars.h"
#include "vtkSphereSource.h"
#include "vtkPlaneSource.h"
#include "vtkSQBOVReader.h"
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
using std::cerr;

#include <string>
using std::string;

int main(int argc, char **argv)
{
  vtkMPIController* controller=vtkMPIController::New();
  controller->Initialize(&argc,&argv,0);
  int worldRank=controller->GetLocalProcessId();
  int worldSize=controller->GetNumberOfProcesses();

  vtkMultiProcessController::SetGlobalController(controller);

  vtkTesting *testHelper = NULL;
  string dataRoot;
  string tempDir;
  string inputFileName;
  string tempFile;
  string tempBaseline;
  string tempDecomp;
  if (worldRank==0)
    {
    testHelper = vtkTesting::New();
    testHelper->AddArguments(argc,const_cast<const char **>(argv));
    if (!testHelper->IsFlagSpecified("-D"))
      {
      cerr << "Error: -D /path/to/data was not specified.";
      return 1;
      }
    dataRoot=testHelper->GetDataRoot();
    inputFileName=dataRoot+"/Data/SciberQuestToolKit/SmallVector/SmallVector.bovm";

    tempDir=testHelper->GetTempDirectory();
    tempBaseline=tempDir+"/SciberQuestToolKit-TestFieldTopologyMapper.png";
    tempDecomp=tempDir+"/SciberQuestToolKit-TestFieldTopologyMapperDecomp.png";
    }
  Broadcast(controller,dataRoot);
  Broadcast(controller,tempDir);
  Broadcast(controller,inputFileName);
  Broadcast(controller,tempBaseline);
  Broadcast(controller,tempDecomp);

  // TODO -- make this a command line option
  enum {
    PLANE=1,
    VOLUME=2
    };
  int seedSelection=PLANE;

  // ooc reader
  vtkSQBOVReader *r=vtkSQBOVReader::New();
  r->SetMetaRead(1);
  r->SetFileName(inputFileName.c_str());
  r->SetPointArrayStatus("vi",1);

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
      vtkPlaneSource *p=vtkPlaneSource::New();
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
      cerr << "Error: invalid seed point selection " << seedSelection << endl;
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
  ftm->SetMasterBlockSize(16);
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

  vtkProcessIdScalars *pid=vtkProcessIdScalars::New();
  pid->SetInputConnection(0,ftm->GetOutputPort(0));
  ftm->Delete();

  vtkDataSetSurfaceFilter *surf=vtkDataSetSurfaceFilter::New();
  surf->SetInputConnection(0,pid->GetOutputPort(0));
  pid->Delete();

  // initialize for domain decomposition
  vtkStreamingDemandDrivenPipeline* exec
    = dynamic_cast<vtkStreamingDemandDrivenPipeline*>(surf->GetExecutive());

  vtkInformation *info=exec->GetOutputInformation(0);

  exec->SetUpdateNumberOfPieces(info,worldSize);
  exec->SetUpdatePiece(info,worldRank);

  // execute the distributed pipeline
  surf->Update();


  // Serial rendering. rank 0 gathers and renders the data in a new
  // pipeline. Other ranks send the output of their pipelines.
  int testStatus = 0;
  const int renderRank=0;
  const int tag=101;
  if (worldRank!=0)
    {
    controller->Send(surf->GetOutput(),renderRank,tag);
    testStatus = 0;
    }
  else
    {
    // gather.
    vtkAppendPolyData *apd=vtkAppendPolyData::New();
    apd->AddInputData(surf->GetOutput());
    for (int i=0; i<worldSize; ++i)
      {
      if (i==renderRank)
        {
        continue;
        }
      vtkPolyData* pd = vtkPolyData::New();
      controller->Receive(pd,vtkMultiProcessController::ANY_SOURCE,tag);
      apd->AddInputData(pd);
      pd->Delete();
      }

    apd->Update();
    /*
    vtkPolyDataWriter *w=vtkPolyDataWriter::New();
    w->SetInputConnection(apd->GetOutputPort(0));
    w->SetFileName("TestFieldTopologyMapper.vtk");
    w->Write();
    w->Delete();
    */

    // render the result
    vtkPolyData *map=vtkPolyData::New();
    map->ShallowCopy(apd->GetOutput());
    map->GetCellData()->SetActiveScalars("IntersectColor");

    vtkPolyDataMapper *pdm;
    pdm=vtkPolyDataMapper::New();
    pdm->SetInputData(map);
    pdm->SetColorModeToMapScalars();
    pdm->SetScalarModeToUseCellData();
    pdm->SetScalarRange(map->GetCellData()->GetArray("IntersectColor")->GetRange());
    map->Delete();

    vtkActor *act;
    act=vtkActor::New();
    act->SetMapper(pdm);
    pdm->Delete();

    vtkRenderer *ren;
    ren=vtkRenderer::New();
    ren->AddActor(act);
    act->Delete();

    vtkCamera *cam=ren->GetActiveCamera();
    cam->SetFocalPoint(0.0,3.5,3.5);
    cam->SetPosition(0.0,15.0,3.5);
    cam->ComputeViewPlaneNormal();
    cam->SetViewUp(0.0,0.0,1.0);
    cam->OrthogonalizeViewUp();
    ren->ResetCamera();

    vtkRenderWindow *rwin;
    rwin=vtkRenderWindow::New();
    rwin->AddRenderer(ren);
    ren->Delete();

    // Perform the regression test.
    int failFlag=vtkTesting::Test(argc, argv, rwin, 5.0);
    if (failFlag==vtkTesting::DO_INTERACTOR)
      {
      // Not testing, interact with scene.
      vtkRenderWindowInteractor *rwi=vtkRenderWindowInteractor::New();
      rwi->SetRenderWindow(rwin);
      rwin->Render();
      rwi->Start();
      rwi->Delete();
      }
    else
      {
      // Save a baseline
      vtkWindowToImageFilter *baselineImage = vtkWindowToImageFilter::New();
      baselineImage->SetInput(rwin);
      vtkPNGWriter *baselineWriter = vtkPNGWriter::New();
      baselineWriter->SetFileName(tempBaseline.c_str());
      baselineWriter->SetInputConnection(baselineImage->GetOutputPort());
      baselineWriter->Write();
      baselineWriter->Delete();
      baselineImage->Delete();

      // save the decomposition, for manula inspection. It can't be
      // part of the baseline since a race condition makes it non
      // deterministic.
      map->GetPointData()->SetActiveScalars("ProcessId");
      pdm->SetScalarModeToUsePointData();
      pdm->SetScalarRange(map->GetPointData()->GetArray("ProcessId")->GetRange());
      pdm->Update();
      rwin->Render();
      vtkWindowToImageFilter *decompImage = vtkWindowToImageFilter::New();
      decompImage->SetInput(rwin);
      vtkPNGWriter *decompWriter = vtkPNGWriter::New();
      decompWriter->SetFileName(tempDecomp.c_str());
      decompWriter->SetInputConnection(decompImage->GetOutputPort());
      decompWriter->Write();
      decompWriter->Delete();
      decompImage->Delete();
      }
    testStatus=failFlag==vtkTesting::PASSED?0:1;
    rwin->Delete();
    }
  surf->Delete();

  controller->Finalize();
  controller->Delete();
  //vtkAlgorithm::SetDefaultExecutivePrototype(0);

  return testStatus;
}
