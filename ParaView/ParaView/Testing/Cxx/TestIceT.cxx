// -*- c++ -*-

#include <vtkCompositeRenderManager.h>
#include <vtkIceTRenderManager.h>
#include "vtkIceTRenderer.h"

#include <vtkMultiProcessController.h>

#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkConeSource.h>
#include <vtkSphereSource.h>
#include <vtkCamera.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#define USE_INTERACTOR 1

#define LOOP
#define ICET


static void Run(vtkMultiProcessController *controller, void *_prm)
{
  vtkParallelRenderManager *prm = (vtkParallelRenderManager *)_prm;

  vtkConeSource* source = vtkConeSource::New();

  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  mapper->SetInput(source->GetOutput());

  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);

  vtkSphereSource * ssource = vtkSphereSource::New();

  vtkPolyDataMapper* smapper = vtkPolyDataMapper::New();
  smapper->SetInput(ssource->GetOutput());

  vtkActor* sactor = vtkActor::New();
  sactor->SetMapper(smapper);

#ifdef ICET
  vtkRenderer *renderer = vtkIceTRenderer::New();
#else
  vtkRenderer *renderer = vtkRenderer::New();
#endif

  //renderer->AddActor(actor);

  vtkRenderWindow *renWin = prm->MakeRenderWindow();
  renWin->AddRenderer(renderer);

  prm->SetRenderWindow(renWin);
  prm->SetController(controller);
  prm->InitializeRMIs();
  prm->InitializePieces();

  cout << "RenWin: " << renWin->GetClassName() << endl;

  int cc = 0;
#ifdef LOOP
  if ( controller->GetLocalProcessId() == 0 )
    {
    while ( 1 )
      {
      cout << "CC: " << cc << endl;
      vtkCamera* cam = renderer->GetActiveCamera();
      //cam->SetFocalPoint(0, 0, 0);
      //cam->SetPosition(0, 0, 5);
      //cam->SetViewUp(0, 1, 0);
      //cam->SetClippingRange(1, 6);
      renWin->Render();
      cam->Azimuth(1);
      if ( cc == 10 )
        {
        cam->Print(cout);
        renderer->AddActor(sactor);
        renderer->ResetCamera();
        cout << "Add actor" << endl;
        }
      if ( cc == 11 )
        {
        cam->Print(cout);
        }
#ifdef _WIN32
      Sleep(100);
#else
      usleep(100000);
#endif
      cc++;
      }
    }
  else
    {
    controller->ProcessRMIs();
    }

#else
  vtkRenderWindowInteractor* inter
    = vtkRenderWindowInteractor::New();
  inter->SetRenderWindow(renWin);

  prm->StartInteractor();
#endif

  renderer->Delete();
  renWin->Delete();
}


int main(int argc, char **argv)
{
  vtkMultiProcessController* controller
    = vtkMultiProcessController::New();
  controller->Initialize(&argc, &argv);
#ifdef ICET
  vtkIceTRenderManager* rm = vtkIceTRenderManager::New();
  rm->SetNumTilesX(2);
  rm->SetNumTilesY(1);
#else
  vtkCompositeRenderManager* rm = vtkCompositeRenderManager::New();
#endif

  controller->SetSingleMethod(Run, rm);
  controller->SingleMethodExecute();

  printf("Exiting program.\n");

  return 0;
}
