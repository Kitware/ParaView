// this is a test to see if triangle strips are created correctly for rendering
// in manta. the problem is a naive implmentation will produce triangles in
// which every other one will be in the wrong orientation.
//
// a cone is rendered in a scene, with the dielectric material. triangle strips
// are used to generate the sides of the cone. if any of the triangles that
// make the cone are in the wrong orientation, then the cone will look
// discontinuous.

// include the required header files for the VTK classes we are using.
#include "vtkCellArray.h"
#include "vtkConeSource.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkMantaActor.h"
#include "vtkMantaCamera.h"
#include "vtkMantaLight.h"
#include "vtkMantaPolyDataMapper.h"
#include "vtkMantaProperty.h"
#include "vtkMantaRenderer.h"
#include "vtkPoints.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStripper.h"

#define NUM_CONE_SIDES 50

void addWalls(vtkRenderer* renderer);
void addCone(vtkRenderer* renderer);
void addLights(vtkRenderer* renderer);

double slate_grey[] = { 0.4392, 0.5020, 0.5647 };

int main(int argc, char** argv)
{
  // create renderer
  vtkMantaRenderer* renderer = vtkMantaRenderer::New();
  renderer->SetBackground(slate_grey[0], slate_grey[1], slate_grey[2]);

  // add the objects in the scene
  addWalls(renderer);
  addCone(renderer);
  addLights(renderer);

  // setup camera
  renderer->ResetCamera();
  vtkCamera* cam = renderer->GetActiveCamera();
  cam->Zoom(1.7);

  // create other rendering stuff
  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->AddRenderer(static_cast<vtkRenderer*>(renderer));
  renWin->SetSize(400, 400);

  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  renWin->Render();

  // set interaction it to trackball style
  vtkInteractorStyleTrackballCamera* style = vtkInteractorStyleTrackballCamera::New();
  iren->SetInteractorStyle(style);

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // clean up
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  style->Delete();

  return !retVal;
}

void addWalls(vtkRenderer* renderer)
{
  // add certain faces of a box to the scene for walls
  const char* material = "phong";
  double box_color[] = { 0.5, 0.2, 0.2 };

  double xmin = -1.0;
  double xmax = 1.0;
  double ymin = -0.896875;
  double ymax = 0.889063;
  double zmin = -0.896875;
  double zmax = 0.889063;

  float pos[8][3];
  pos[0][0] = xmin;
  pos[0][1] = ymin;
  pos[0][2] = zmin;
  pos[1][0] = xmax;
  pos[1][1] = ymin;
  pos[1][2] = zmin;
  pos[2][0] = xmax;
  pos[2][1] = ymax;
  pos[2][2] = zmin;
  pos[3][0] = xmin;
  pos[3][1] = ymax;
  pos[3][2] = zmin;
  pos[4][0] = xmin;
  pos[4][1] = ymin;
  pos[4][2] = zmax;
  pos[5][0] = xmax;
  pos[5][1] = ymin;
  pos[5][2] = zmax;
  pos[6][0] = xmax;
  pos[6][1] = ymax;
  pos[6][2] = zmax;
  pos[7][0] = xmin;
  pos[7][1] = ymax;
  pos[7][2] = zmax;

  int numSides = 3;
  static vtkIdType pointIds[][4] = {
    { 0, 1, 2, 3 }, // back
    //{4, 5, 6, 7}, // front
    { 0, 1, 5, 4 }, // bottom
                    //{1, 2, 6, 5}, // right
    //{2, 3, 7, 6}, // top
    { 3, 0, 4, 7 }, // left
  };

  // We'll create the building blocks of polydata including data attributes.
  vtkPolyData* cube = vtkPolyData::New();
  vtkPoints* points = vtkPoints::New();
  vtkCellArray* polys = vtkCellArray::New();

  // Load the point, cell, and data attributes.
  int i;
  for (i = 0; i < 8; i++)
    points->InsertPoint(i, pos[i]);
  for (i = 0; i < numSides; i++)
    polys->InsertNextCell(4, pointIds[i]);

  // We now assign the pieces to the vtkPolyData.
  cube->SetPoints(points);
  points->Delete();
  cube->SetPolys(polys);
  polys->Delete();

  // Now we'll look at it.
  vtkMantaPolyDataMapper* cubeMapper = vtkMantaPolyDataMapper::New();
  cubeMapper->SetInput(cube);
  vtkMantaActor* cubeActor = vtkMantaActor::New();
  cubeActor->SetMapper(cubeMapper);

  vtkProperty* property = cubeActor->GetProperty();
  property->SetDiffuseColor(box_color);
  property->SetSpecularColor(box_color);

  vtkMantaProperty* mantaProperty = vtkMantaProperty::SafeDownCast(cubeActor->GetProperty());
  mantaProperty->SetMaterialType(material);

  renderer->AddActor(cubeActor);
}

void addCone(vtkRenderer* renderer)
{
  // add a cone to the scene
  vtkConeSource* cone = vtkConeSource::New();
  cone->SetHeight(1.0);
  cone->SetRadius(0.5);
  cone->SetResolution(NUM_CONE_SIDES);

  // use vtkStripper to ensure that triangle strips are used
  vtkStripper* strip = vtkStripper::New();
  strip->SetInputConnection(cone->GetOutputPort());

  vtkMantaPolyDataMapper* coneMapper = vtkMantaPolyDataMapper::New();
  coneMapper->SetInputConnection(strip->GetOutputPort());

  vtkMantaActor* coneActor = vtkMantaActor::New();
  coneActor->SetMapper(coneMapper);

  vtkProperty* coneProperty = coneActor->GetProperty();
  coneProperty->SetColor(1.0, 0.3882, 0.2784);
  coneProperty->SetDiffuse(0.7);
  coneProperty->SetSpecular(0.4);
  coneProperty->SetSpecularPower(20);
  coneActor->SetProperty(coneProperty);

  vtkMantaProperty* mantaProperty = vtkMantaProperty::SafeDownCast(coneProperty);
  mantaProperty->SetMaterialType("dielectric");
  mantaProperty->SetN(1.0);
  mantaProperty->SetNt(1.3);

  renderer->AddActor(coneActor);
}

void addLights(vtkRenderer* renderer)
{
  // light in front of camera
  vtkMantaLight* light1 = vtkMantaLight::New();
  light1->PositionalOn();
  light1->SetPosition(renderer->GetActiveCamera()->GetPosition());
  light1->SetFocalPoint(renderer->GetActiveCamera()->GetFocalPoint());
  light1->SetColor(0.5, 0.5, 0.5);
  light1->SetLightTypeToCameraLight();
  renderer->SetLightFollowCamera(1);
  renderer->AddLight(light1);

  // light in upper right
  vtkMantaLight* light2 = vtkMantaLight::New();
  light2->PositionalOn();
  light2->SetPosition(5, 5, 5);
  light2->SetColor(0.5, 0.5, 0.5);
  light2->SetLightTypeToCameraLight();
  renderer->SetLightFollowCamera(1);
  renderer->AddLight(light2);

  // light straight up, looking down
  vtkMantaLight* light3 = vtkMantaLight::New();
  light3->PositionalOn();
  light3->SetPosition(0.5, 5, 0.5);
  light3->SetFocalPoint(0, 0, 0);
  light3->SetColor(0.5, 0.5, 0.5);
  light3->SetLightTypeToCameraLight();
  renderer->SetLightFollowCamera(1);
  renderer->AddLight(light3);

  // light looking down the z-axis
  vtkMantaLight* light4 = vtkMantaLight::New();
  light4->PositionalOn();
  light4->SetPosition(-5, 0, 0);
  light4->SetFocalPoint(0, 0, 0);
  light4->SetColor(0.4, 0.4, 0.4);
  light4->SetLightTypeToCameraLight();
  renderer->SetLightFollowCamera(1);
  renderer->AddLight(light4);
}
