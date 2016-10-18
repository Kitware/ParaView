/*
Tests whether we can give manta non aligned image sizes without getting staircases back.
*/

#include "vtkConeSource.h"
#include "vtkCylinderSource.h"
#include "vtkMantaActor.h"
#include "vtkMantaPolyDataMapper.h"
#include "vtkMantaRenderer.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"

#ifndef usleep
#define usleep(time)
#endif

// this program tests creating odd-width images

//----------------------------------------------------------------------------
int OddWidthImages(int argc, char* argv[])
{
  int objRes = 12;
  double objRad = 0.075;

  // cone
  vtkConeSource* cone = vtkConeSource::New();
  cone->SetRadius(objRad);
  cone->SetHeight(objRad * 2);
  cone->SetResolution(objRes);

  vtkMantaPolyDataMapper* coneMapper = vtkMantaPolyDataMapper::New();
  coneMapper->SetInputConnection(cone->GetOutputPort());

  vtkMantaActor* coneActor = vtkMantaActor::New();
  coneActor->SetMapper(coneMapper);
  coneActor->AddPosition(-objRad * 3.0, 0.0, 0.0);
  coneActor->RotateZ(90.0);

  // sphere
  vtkSphereSource* sphere = vtkSphereSource::New();
  sphere->SetCenter(0.0, 0.0, 0.0);
  sphere->SetRadius(objRad);
  sphere->SetThetaResolution(objRes);
  sphere->SetPhiResolution(objRes);

  vtkMantaPolyDataMapper* sphereMapper = vtkMantaPolyDataMapper::New();
  sphereMapper->SetInputConnection(sphere->GetOutputPort());

  vtkMantaActor* sphereActor = vtkMantaActor::New();
  sphereActor->SetMapper(sphereMapper);

  // cylinder
  vtkCylinderSource* cylinder = vtkCylinderSource::New();
  cylinder->SetCenter(objRad * 3, 0.0, 0.0);
  cylinder->SetRadius(objRad);
  cylinder->SetHeight(objRad * 2);
  cylinder->SetResolution(objRes);

  vtkMantaPolyDataMapper* cylinderMapper = vtkMantaPolyDataMapper::New();
  cylinderMapper->SetInputConnection(cylinder->GetOutputPort());

  vtkMantaActor* cylinderActor = vtkMantaActor::New();
  cylinderActor->SetMapper(cylinderMapper);

  vtkMantaRenderer* renderer = vtkMantaRenderer::New();
  renderer->SetBackground(0.0, 0.0, 1.0);
  renderer->AddActor(coneActor);
  renderer->AddActor(sphereActor);
  renderer->AddActor(cylinderActor);

  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->AddRenderer(renderer);
  renWin->SetSize(400, 400);

  vtkRenderWindowInteractor* interactor = vtkRenderWindowInteractor::New();
  interactor->SetRenderWindow(renWin);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }
  else
  {
    for (int i = 0; i < 6; i++)
    {
      renWin->SetSize(400 + 51 * i, 400);
      renWin->Render();
      if (i == 5)
      {
        retVal = vtkRegressionTestImage(renWin);
      }
      usleep(1000000);
    }
  }

  cone->Delete();
  coneMapper->Delete();
  coneActor->Delete();

  sphere->Delete();
  sphereMapper->Delete();
  sphereActor->Delete();

  cylinder->Delete();
  cylinderMapper->Delete();
  cylinderActor->Delete();

  renderer->Delete();
  renWin->Delete();
  interactor->Delete();

  if (retVal == 0)
  {
    // someone is intercepting the return value which prevents any failures
    cerr << "FAILURE" << endl;
    exit(1);
  }
  cerr << "SUCCESS" << endl;
  return 0;
}
