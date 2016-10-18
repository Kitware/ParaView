// given .vtk polydata files as arguments, loads the meshes and puts them in a scene
// rendered using Manta

#include <stdlib.h>

// include the required header files for the VTK classes we are using.
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLight.h"
#include "vtkMantaActor.h"
#include "vtkMantaCamera.h"
#include "vtkMantaLight.h"
#include "vtkMantaPolyDataMapper.h"
#include "vtkMantaProperty.h"
#include "vtkMantaRenderer.h"
#include "vtkPolyDataReader.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

//#include <Engine/Control/RTRT.h>

#define NUM_COLORS 2
// double colors[2][3] = { {0.9412, 0.9020, 0.5490},
double colors[2][3] = { { 0.6421, 0.6020, 0.1490 }, { 0.9000, 0.9000, 0.9000 } };
//{0.5000, 0.2882, 0.1784}};

double checkerColors[2][3] = { { 0.9412, 0.2020, 0.1490 }, { 0.7000, 0.7000, 0.7000 } };

double banana[] = { 0.8900, 0.8100, 0.3400 };
double slate_grey[] = { 0.4392, 0.5020, 0.5647 };
double lamp_black[] = { 0.1800, 0.2800, 0.2300 };

int main(int argc, char** argv)
{
  int i;

  if (argc < 2)
  {
    printf("usage: %s <file1> <file2> ...\n", argv[0]);
    printf("Only .vtk polygonal files are supported\n");
    printf("exiting...\n");
    exit(-1);
  }

  // all arguments should be filenames to .vtk files
  int numInputs = argc - 1;

  // get the filenames
  char** filenames = (char**)malloc(sizeof(char*) * numInputs);
  for (i = 0; i < numInputs; i++)
  {
    filenames[i] = argv[i + 1];
  }

  // load the .vtk files containing geometry information
  vtkPolyDataReader** readers = (vtkPolyDataReader**)malloc(sizeof(vtkPolyDataReader*) * numInputs);
  for (i = 0; i < numInputs; i++)
  {
    readers[i] = vtkPolyDataReader::New();
    readers[i]->SetFileName(filenames[i]);
  }

  vtkMantaPolyDataMapper** dataMappers =
    (vtkMantaPolyDataMapper**)malloc(sizeof(vtkMantaPolyDataMapper*) * numInputs);
  for (i = 0; i < numInputs; i++)
  {
    dataMappers[i] = vtkMantaPolyDataMapper::New();
    dataMappers[i]->SetInputConnection(readers[i]->GetOutputPort());
    dataMappers[i]->ScalarVisibilityOff();
  }

  // create actors for each file
  vtkMantaActor** actors = (vtkMantaActor**)malloc(sizeof(vtkMantaActor*) * numInputs);
  for (i = 0; i < numInputs; i++)
  {
    actors[i] = vtkMantaActor::New();
    actors[i]->SetMapper(dataMappers[i]);

    // the index of the color to use
    int j = i % NUM_COLORS;

    vtkMantaProperty* property = vtkMantaProperty::SafeDownCast(actors[i]->GetProperty());

    property->SetDiffuseColor(colors[j][0], colors[j][1], colors[j][2]);
    property->SetSpecularColor(0.5, 0.5, 0.5);
    property->SetSpecular(0.2);
    property->SetSpecularPower(100);
    // property->SetOpacity(0.9);
    property->SetMaterialType("phong");
    // property->SetMaterialType("thindielectric");
    property->SetEta(2.52);
    property->SetThickness(0.4);
  }

  // create the renderer
  vtkMantaRenderer* renderer = vtkMantaRenderer::New();
  for (i = 0; i < numInputs; i++)
  {
    renderer->AddActor(actors[i]);
  }

  renderer->SetBackground(slate_grey[0], slate_grey[1], slate_grey[2]);

  vtkMantaCamera* cam = vtkMantaCamera::New();
  renderer->SetActiveCamera(cam);
  renderer->ResetCamera();

  // create lights
  vtkMantaLight* light1 = vtkMantaLight::New();
  light1->PositionalOn();
  light1->SetPosition(renderer->GetActiveCamera()->GetPosition());
  light1->SetFocalPoint(renderer->GetActiveCamera()->GetFocalPoint());
  light1->SetColor(0.6, 0.6, 0.6);
  light1->SetLightTypeToCameraLight();
  renderer->SetLightFollowCamera(1);
  renderer->AddLight(light1);

  vtkMantaLight* light2 = vtkMantaLight::New();
  light2->PositionalOn();
  light2->SetPosition(1, 1, 0);
  light2->SetFocalPoint(0, 0, 0);
  light2->SetColor(0.6, 0.6, 0.6);
  light2->SetLightTypeToCameraLight();
  renderer->SetLightFollowCamera(1);
  renderer->AddLight(light2);

  // create the render window
  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->AddRenderer(static_cast<vtkRenderer*>(renderer));
  renWin->SetSize(400, 400);

  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // set it to trackball style
  vtkInteractorStyleTrackballCamera* style = vtkInteractorStyleTrackballCamera::New();
  iren->SetInteractorStyle(style);

  renWin->Render();
  iren->Start();

  // cleanup
  for (i = 0; i < numInputs; i++)
  {
    readers[i]->Delete();
    dataMappers[i]->Delete();
    actors[i]->Delete();
  }

  free(filenames);
  free(readers);
  free(dataMappers);
  free(actors);

  renderer->Delete();
  renWin->Delete();
  cam->Delete();
  light1->Delete();
  light2->Delete();
  iren->Delete();
  style->Delete();

  return 0;
}
