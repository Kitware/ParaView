// this program tests creating and deleting objects

#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkCylinderSource.h"
#include "vtkMantaActor.h"
#include "vtkMantaPolyDataMapper.h"
#include "vtkMantaRenderer.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

#include <unistd.h>

bool useGL = false;

vtkPolyDataMapper* makeMapper()
{
  if (useGL)
  {
    return vtkPolyDataMapper::New();
  }
  else
  {
    return vtkMantaPolyDataMapper::New();
  }
}

vtkActor* makeActor()
{
  if (useGL)
  {
    return vtkActor::New();
  }
  else
  {
    return vtkMantaActor::New();
  }
}

vtkRenderer* makeRenderer()
{
  if (useGL)
  {
    return vtkRenderer::New();
  }
  else
  {
    return vtkMantaRenderer::New();
  }
}

vtkRenderWindow* makeRenderWindow()
{
  return vtkRenderWindow::New();
}

//----------------------------------------------------------------------------
int CreateDeleteObjects(int argc, char* argv[])
{
  int objRes = 12;
  double objRad = 0.075;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-useGL"))
    {
      useGL = true;
    }
  }

  vtkRenderer* renderer = makeRenderer();
  renderer->SetBackground(0.0, 0.0, 1.0);

  vtkRenderWindow* renWin = makeRenderWindow();
  renWin->AddRenderer(renderer);
  renWin->SetSize(400, 400);

  // create cone
  vtkConeSource* cone = vtkConeSource::New();
  cone->SetRadius(objRad);
  cone->SetHeight(objRad * 2);
  cone->SetResolution(objRes);

  vtkPolyDataMapper* coneMapper = makeMapper();
  coneMapper->SetInputConnection(cone->GetOutputPort());

  vtkActor* coneActor = makeActor();
  coneActor->SetMapper(coneMapper);
  coneActor->AddPosition(0.0, objRad * 2.0, 0.0);
  coneActor->RotateZ(90.0);

  renderer->AddActor(coneActor);

  // create sphere
  vtkSphereSource* sphere = vtkSphereSource::New();
  sphere->SetCenter(0.0, 0.0, 0.0);
  sphere->SetRadius(objRad);
  sphere->SetThetaResolution(objRes);
  sphere->SetPhiResolution(objRes);

  vtkPolyDataMapper* sphereMapper = makeMapper();
  sphereMapper->SetInputConnection(sphere->GetOutputPort());

  vtkActor* sphereActor = makeActor();
  sphereActor->SetMapper(sphereMapper);

  renderer->AddActor(sphereActor);

  // create cylinder
  vtkCylinderSource* cylinder = vtkCylinderSource::New();
  cylinder->SetCenter(0.0, -objRad * 2, 0.0);
  cylinder->SetRadius(objRad);
  cylinder->SetHeight(objRad * 2);
  cylinder->SetResolution(objRes);

  vtkPolyDataMapper* cylinderMapper = makeMapper();
  cylinderMapper->SetInputConnection(cylinder->GetOutputPort());

  vtkActor* cylinderActor = makeActor();
  cylinderActor->SetMapper(cylinderMapper);

  renderer->AddActor(cylinderActor);

  renWin->Render();

  usleep(100000);

  // delete cone
  cerr << "DELETE CONE" << endl;
  renderer->RemoveActor(coneActor);
  cone->Delete();
  coneMapper->Delete();
  coneActor->Delete();

  renWin->Render();

  usleep(100000);

  // delete sphere
  cerr << "DELETE SPHERE" << endl;
  renderer->RemoveActor(sphereActor);
  sphere->Delete();
  sphereMapper->Delete();
  sphereActor->Delete();

  renWin->Render();
  usleep(100000);

  // delete cylinder
  cerr << "DELETE CYLINDER" << endl;
  renderer->RemoveActor(cylinderActor);
  cylinder->Delete();
  cylinderMapper->Delete();
  cylinderActor->Delete();

  renWin->Render();
  usleep(100000);

  // re create sphere
  cerr << "CREATE NEW SPHERE" << endl;
  sphere = vtkSphereSource::New();
  sphere->SetCenter(0.0, 0.0, 0.0);
  sphere->SetRadius(objRad);
  sphere->SetThetaResolution(objRes);
  sphere->SetPhiResolution(objRes);

  sphereMapper = makeMapper();
  sphereMapper->SetInputConnection(sphere->GetOutputPort());

  sphereActor = makeActor();
  sphereActor->SetMapper(sphereMapper);

  renderer->AddActor(sphereActor);

  renWin->Render();
  usleep(100000);

  cerr << "DELETE NEW SPHERE" << endl;

  renderer->RemoveActor(sphereActor);

  renWin->Render();
  usleep(100000);

  renWin->Render();
  usleep(100000);

  renWin->Render();
  usleep(100000);

  // re-delete sphere
  sphere->Delete();
  sphereMapper->Delete();
  sphereActor->Delete();

  renderer->Delete();
  renWin->Delete();

  // if vtkManta isn't deallocating Manta objects safely,
  // we would have crashed before now.
  return 0;
}
