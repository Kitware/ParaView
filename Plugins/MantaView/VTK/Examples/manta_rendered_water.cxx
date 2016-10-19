// this scene has a tank with two liquids, and a surface between the interface
// of the two liquids

#include <stdlib.h>
#include <string.h>

// include the required header files for the VTK classes we are using.
#include "vtkCellArray.h"
#include "vtkCleanPolyData.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkMantaActor.h"
#include "vtkMantaCamera.h"
#include "vtkMantaLight.h"
#include "vtkMantaPolyDataMapper.h"
#include "vtkMantaRenderer.h"
#include "vtkParametricFunctionSource.h"
#include "vtkParametricRandomHills.h"
#include "vtkPoints.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkTransformPolyDataFilter.h"

#include "vtkMantaProperty.h"

void addBox(vtkRenderer* renderer);
void addLights(vtkRenderer* renderer);
void addHills(vtkRenderer* renderer);

void addLowerWater(vtkRenderer* renderer);
vtkMapper* getLowerLeftWater();
vtkMapper* getLowerRightWater();
vtkMapper* getLowerFrontWater();
vtkMapper* getLowerBackWater();

void addUpperWater(vtkRenderer* renderer);
vtkMapper* getUpperLeftWater();
vtkMapper* getUpperRightWater();
vtkMapper* getUpperFrontWater();
vtkMapper* getUpperBackWater();

vtkTransformPolyDataFilter* transformHill(vtkParametricFunctionSource* source, bool isBack = false);
vtkParametricFunctionSource* getRandomHill(double umin, double umax, double vmin, double vmax);
vtkMapper* generateSidePolygon(vtkParametricFunctionSource* source, bool isLower, const char* side);

double slate_grey[] = { 0.4392, 0.5020, 0.5647 };
double blue[] = { 0.4235, 0.8745, 0.9176 };

// the seed for the random number generator of the random hills
int randomHillSeed = 90282029;

// how fine the random hills are sampled
int hillResolution = 100;

int main(int argc, char** argv)
{
  // create renderer and add objects
  vtkMantaRenderer* renderer = vtkMantaRenderer::New();
  renderer->SetBackground(slate_grey[0], slate_grey[1], slate_grey[2]);

  // add the objects in the scene
  addBox(renderer);
  addHills(renderer);
  addLowerWater(renderer);
  addUpperWater(renderer);

  vtkMantaCamera* camera = vtkMantaCamera::New();
  renderer->SetActiveCamera(camera);
  renderer->ResetCamera();

  // create lights
  addLights(renderer);

  // create the render window
  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->AddRenderer(static_cast<vtkRenderer*>(renderer));
  renWin->SetSize(400, 400);

  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // set interaction it to trackball style
  vtkInteractorStyleTrackballCamera* style = vtkInteractorStyleTrackballCamera::New();
  iren->SetInteractorStyle(style);

  renWin->Render();
  iren->Start();

  // clean up
  camera->Delete();
  renderer->Delete();
  renWin->Delete();
  style->Delete();
  iren->Delete();

  return 0;
}

void addBox(vtkRenderer* renderer)
{
  // add certain faces of a cube
  const char* material = "dielectric";
  double box_color[] = { 0.9, 0.2, 0.2 };

  float pointPositions[8][3] = { { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 }, { 0, 0, 1 },
    { 1, 0, 1 }, { 1, 1, 1 }, { 0, 1, 1 } };

  int numSides = 2;
  static vtkIdType pointIds[][4] = {
    //{0, 1, 2, 3}, // back
    //{4, 5, 6, 7}, // front
    { 0, 1, 5, 4 }, // bottom
                    //{1, 2, 6, 5}, // right
    { 2, 3, 7, 6 }, // top
                    //{3, 0, 4, 7}, // left
  };

  // We'll create the building blocks of polydata including data attributes.
  vtkPolyData* cube = vtkPolyData::New();
  vtkPoints* points = vtkPoints::New();
  vtkCellArray* polys = vtkCellArray::New();

  // Load the point, cell, and data attributes.
  int i;
  for (i = 0; i < 8; i++)
    points->InsertPoint(i, pointPositions[i]);
  for (i = 0; i < numSides; i++)
    polys->InsertNextCell(4, pointIds[i]);

  // We now assign the pieces to the vtkPolyData.
  cube->SetPoints(points);
  points->Delete();
  cube->SetPolys(polys);
  polys->Delete();

  // Now we'll look at it.
  vtkMantaPolyDataMapper* cubeMapper = vtkMantaPolyDataMapper::New();
  cubeMapper->SetInputData(cube);
  vtkMantaActor* cubeActor = vtkMantaActor::New();
  cubeActor->SetMapper(cubeMapper);

  vtkMantaProperty* property = vtkMantaProperty::SafeDownCast(cubeActor->GetProperty());
  property->SetDiffuseColor(box_color);
  property->SetMaterialType(material);
  property->SetN(1);
  property->SetNt(1);

  renderer->AddActor(cubeActor);
}

void addHills(vtkRenderer* renderer)
{
  // render the hills on the fly
  const char* material = "phong";
  double color[] = { 0.7, 0.1, 0.1 };

  // generate random hills
  vtkParametricRandomHills* hills = vtkParametricRandomHills::New();
  hills->SetRandomSeed(randomHillSeed);
  hills->SetMinimumU(-10);
  hills->SetMaximumU(10);
  hills->SetMinimumV(-10);
  hills->SetMaximumV(10);

  vtkParametricFunctionSource* hillsource = vtkParametricFunctionSource::New();
  hillsource->SetParametricFunction(hills);

  hillsource->SetUResolution(hillResolution);
  hillsource->SetVResolution(hillResolution);
  hillsource->SetWResolution(hillResolution);

  // transform the hills so it fits in a cube
  vtkTransformPolyDataFilter* transformFilter = transformHill(hillsource);

  // do the usual mapping
  vtkMantaPolyDataMapper* mapper = vtkMantaPolyDataMapper::New();
  mapper->SetInputConnection(transformFilter->GetOutputPort());

  vtkMantaActor* actor = vtkMantaActor::New();
  actor->SetMapper(mapper);

  vtkMantaProperty* property = vtkMantaProperty::SafeDownCast(actor->GetProperty());
  property->SetDiffuseColor(color);
  property->SetSpecularColor(1.0, 1.0, 1.0);
  property->SetSpecularPower(100);
  property->SetMaterialType(material);

  renderer->AddActor(actor);
}

vtkTransformPolyDataFilter* transformHill(vtkParametricFunctionSource* source, bool isBack)
{
  // given a source for a random hill that uses the default parameters,
  // transform it so it fits in a cube spanning from 0 to 1 in all dimensions

  // these numbers are a total hack, but they work.
  // back faces need to be transformed differently
  vtkTransform* transform = vtkTransform::New();
  if (isBack)
  {
    transform->Translate(0.5, -0.0024149403907358646 + 0.35, 0);
  }
  else
  {
    transform->Translate(0.5, -0.0024149403907358646 + 0.35, 1);
  }
  transform->Scale(0.05, 0.05, 0.05);
  transform->RotateX(-90);

  vtkTransformPolyDataFilter* transformFilter = vtkTransformPolyDataFilter::New();
  transformFilter->SetTransform(transform);
  transformFilter->SetInputConnection(source->GetOutputPort());

  return transformFilter;
}

void addLowerWater(vtkRenderer* renderer)
{
  // compute the four sides of the lower water such that the top edges
  // of the sides are flush with the random hill
  int i;
  double color[] = { 0.8, 0.8, 0.9 };
  const char* material = "dielectric";
  float eta = 1.5f;
  float thickness = 0.1f;
  float n = 1.0f;
  float nt = 1.33f;
  vtkMapper* mapper[4];

  mapper[0] = getLowerLeftWater();
  mapper[1] = getLowerRightWater();
  mapper[2] = getLowerFrontWater();
  mapper[3] = getLowerBackWater();

  for (i = 0; i < 4; i++)
  {
    vtkMantaActor* actor = vtkMantaActor::New();
    actor->SetMapper(mapper[i]);

    vtkMantaProperty* property = vtkMantaProperty::SafeDownCast(actor->GetProperty());
    property->SetDiffuseColor(color);
    property->SetMaterialType(material);
    property->SetEta(eta);
    property->SetThickness(thickness);
    property->SetN(n);
    property->SetNt(nt);

    renderer->AddActor(actor);
  }
}

vtkMapper* getLowerLeftWater()
{
  // get the left side of the lower water

  // generate the left curve
  vtkParametricFunctionSource* source = getRandomHill(-10, -10, -10, 10);

  // generate the polygon, return the mapper
  return generateSidePolygon(source, true, "left");
}

vtkMapper* getLowerRightWater()
{
  // get the right side of the lower water

  // generate the right curve
  vtkParametricFunctionSource* source = getRandomHill(10, 10, -10, 10);

  // generate the polygon, return the mapper
  return generateSidePolygon(source, true, "right");
}

vtkMapper* getLowerFrontWater()
{
  // get the front side of the lower water

  // generate the front curve
  vtkParametricFunctionSource* source = getRandomHill(-10, 10, 10, 10);

  // generate the polygon, return the mapper
  return generateSidePolygon(source, true, "front");
}

vtkMapper* getLowerBackWater()
{
  // get the back side of the lower water

  // generate the back curve
  vtkParametricFunctionSource* source = getRandomHill(-10, 10, -10, -10);

  // generate the polygon, return the mapper
  return generateSidePolygon(source, true, "back");
}

void addUpperWater(vtkRenderer* renderer)
{
  // compute the four sides of the upper water such that the bottom
  // edges of the sides are flush with the random hill
  int i;
  double color[] = { 0.7, 0.9, 0.7 };
  const char* material = "dielectric";
  float eta = 1.5f;
  float thickness = 0.1f;
  float n = 1.0f;
  float nt = 1.33f;
  vtkMapper* mapper[4];

  mapper[0] = getUpperLeftWater();
  mapper[1] = getUpperRightWater();
  mapper[2] = getUpperFrontWater();
  mapper[3] = getUpperBackWater();

  for (i = 0; i < 4; i++)
  {
    vtkMantaActor* actor = vtkMantaActor::New();
    actor->SetMapper(mapper[i]);

    vtkMantaProperty* property = vtkMantaProperty::SafeDownCast(actor->GetProperty());
    property->SetDiffuseColor(color);
    property->SetMaterialType(material);
    property->SetEta(eta);
    property->SetThickness(thickness);
    property->SetN(n);
    property->SetNt(nt);

    renderer->AddActor(actor);
  }
}

vtkMapper* getUpperLeftWater()
{
  // get the left side of the upper water

  // generate the left curve
  vtkParametricFunctionSource* source = getRandomHill(-10, -10, -10, 10);

  // generate the polygon, return the mapper
  return generateSidePolygon(source, false, "left");
}

vtkMapper* getUpperRightWater()
{
  // get the right side of the upper water

  // generate the right curve
  vtkParametricFunctionSource* source = getRandomHill(10, 10, -10, 10);

  // generate the polygon, return the mapper
  return generateSidePolygon(source, false, "right");
}

vtkMapper* getUpperFrontWater()
{
  // get the front side of the upper water

  // generate the front curve
  vtkParametricFunctionSource* source = getRandomHill(-10, 10, 10, 10);

  // generate the polygon, return the mapper
  return generateSidePolygon(source, false, "front");
}

vtkMapper* getUpperBackWater()
{
  // get the back side of the upper water

  // generate the back curve
  vtkParametricFunctionSource* source = getRandomHill(-10, 10, -10, -10);

  // generate the polygon, return the mapper
  return generateSidePolygon(source, false, "back");
}

vtkParametricFunctionSource* getRandomHill(double umin, double umax, double vmin, double vmax)
{
  // given the max and min values to sample over, return a random hill
  // sampled over those ranges.
  vtkParametricRandomHills* hills = vtkParametricRandomHills::New();
  hills->SetRandomSeed(randomHillSeed);

  vtkParametricFunctionSource* hillsource = vtkParametricFunctionSource::New();
  hillsource->SetParametricFunction(hills);

  hillsource->SetUResolution(hillResolution);
  hillsource->SetVResolution(hillResolution);
  hillsource->SetWResolution(hillResolution);

  hills->SetMinimumU(umin);
  hills->SetMaximumU(umax);
  hills->SetMinimumV(vmin);
  hills->SetMaximumV(vmax);

  return hillsource;
}

vtkMapper* generateSidePolygon(vtkParametricFunctionSource* source, bool isLower, const char* side)
{
  // given a source for a random hill, generate the side polygon.
  // the isLower argument determines if it is generating a polygon
  // for the lower half or upper half.
  // the side argument determines which side is being made. possible
  // values are "left", "right", "front", "back".
  // basically given a curve, make quads that take two adjacent points on
  // the curve, and make a quad that goes straight up or down the cube.

  // transform the hill so it fits in a cube
  // Note: the back side need to be transformed differently
  vtkTransformPolyDataFilter* transformFilter;
  if (strcmp(side, "back") == 0)
  {
    transformFilter = transformHill(source, true);
  }
  else
  {
    transformFilter = transformHill(source, false);
  }

  // clean data, take out duplicate points
  vtkCleanPolyData* cleaner = vtkCleanPolyData::New();
  cleaner->SetInputConnection(transformFilter->GetOutputPort());
  cleaner->Update();

  // add point to vtkPoints object
  vtkPolyData* poly = cleaner->GetOutput();
  int numPointsOnCurve = poly->GetNumberOfPoints();
  vtkPoints* points = vtkPoints::New();
  int i;
  for (i = 0; i < numPointsOnCurve; i++)
  {
    points->InsertNextPoint(poly->GetPoint(i));
  }

  // figure out whether extra points will run along the bottom or top
  float y = isLower ? 0 : 1;

  // add points that are in the same spots as the curve, but projected either
  // to the bottom or top of the cube
  for (i = 0; i < numPointsOnCurve; i++)
  {
    double* p = poly->GetPoint(i);
    points->InsertNextPoint(p[0], y, p[2]);
  }

  // make quads that basically go straight up and down
  vtkCellArray* polys = vtkCellArray::New();

  // need the geometry to have clockwise orientation for refraction
  // and reflection to work correctly
  bool clockwise = true;
  if (strcmp(side, "back") == 0 || strcmp(side, "right") == 0)
  {
    clockwise = false;
  }
  if (!isLower)
  {
    clockwise = !clockwise;
  }

  if (clockwise)
  {
    for (i = 0; i < numPointsOnCurve - 1; i++)
    {
      polys->InsertNextCell(4);
      polys->InsertCellPoint(i);
      polys->InsertCellPoint(i + 1);
      polys->InsertCellPoint(i + 1 + numPointsOnCurve);
      polys->InsertCellPoint(i + numPointsOnCurve);
    }
  }
  else
  {
    for (i = 0; i < numPointsOnCurve - 1; i++)
    {
      polys->InsertNextCell(4);
      polys->InsertCellPoint(i + 1);
      polys->InsertCellPoint(i);
      polys->InsertCellPoint(i + numPointsOnCurve);
      polys->InsertCellPoint(i + 1 + numPointsOnCurve);
    }
  }

  vtkPolyData* newPoly = vtkPolyData::New();
  newPoly->SetPoints(points);
  newPoly->SetPolys(polys);

  vtkMantaPolyDataMapper* mapper = vtkMantaPolyDataMapper::New();
  mapper->SetInputData(newPoly);

  return mapper;
}

void addLights(vtkRenderer* renderer)
{
  // light in front of camera
  vtkMantaLight* light1 = vtkMantaLight::New();
  light1->PositionalOn();
  light1->SetPosition(renderer->GetActiveCamera()->GetPosition());
  light1->SetFocalPoint(renderer->GetActiveCamera()->GetFocalPoint());
  light1->SetColor(1.0, 1.0, 1.0);
  light1->SetLightTypeToCameraLight();
  renderer->SetLightFollowCamera(1);
  renderer->AddLight(light1);

  // light in upper right
  vtkMantaLight* light2 = vtkMantaLight::New();
  light2->PositionalOn();
  light2->SetPosition(5, 5, 5);
  light2->SetColor(1.0, 1.0, 1.0);
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
}
