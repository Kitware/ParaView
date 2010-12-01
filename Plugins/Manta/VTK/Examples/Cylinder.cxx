#include "vtkCylinderSource.h"
#include "vtkMantaPolyDataMapper.h"
#include "vtkMantaProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkMantaCamera.h"
#include "vtkMantaActor.h"
#include "vtkMantaRenderer.h"
#include "vtkRenderWindowInteractor.h"

double lamp_black[]     = {0.1800, 0.2800, 0.2300};

int main()
{

  vtkCylinderSource *source1 = vtkCylinderSource::New();
  source1->SetHeight(3.0);
  source1->SetRadius(1.0);
  source1->SetCenter(1,1,1);
  source1->SetResolution(5);

  vtkPolyDataMapper *mapper1 = vtkPolyDataMapper::New();
  mapper1->SetInputConnection(source1->GetOutputPort());

  vtkActor *actor1 = vtkActor::New();
  actor1->SetMapper(mapper1);

  vtkProperty *property1 = actor1->GetProperty();
  property1->SetColor(0.0, 1.0, 0.0);
  property1->SetDiffuse(0.7);
  property1->SetSpecular(0.4);
  property1->SetSpecularPower(20);
  property1->SetInterpolationToFlat();
  actor1->SetProperty(property1);

  vtkRenderer *glrenderer= vtkRenderer::New();
  glrenderer->SetBackground(0.0, 0.0, 0.0);
  //glrenderer->EraseOff();
  glrenderer->AddActor(actor1);

  vtkCylinderSource *source2 = vtkCylinderSource::New();
  source2->SetHeight(3.0);
  source2->SetRadius(1.0);
  source2->SetResolution(5);

  vtkMantaPolyDataMapper *mapper2 = vtkMantaPolyDataMapper::New();
  mapper2->SetInputConnection(source2->GetOutputPort());

  vtkMantaActor *actor2 = vtkMantaActor::New();
  actor2->SetMapper(mapper2);

  vtkProperty *property2 = actor2->GetProperty();
  property2->SetColor(1.0, 0.3882, 0.2784);
  property2->SetDiffuse(0.7);
  property2->SetSpecular(0.4);
  property2->SetSpecularPower(20);
  property2->SetInterpolationToFlat();
  actor2->SetProperty(property2);

  vtkMantaRenderer *mantarenderer= vtkMantaRenderer::New();
  mantarenderer->AddActor(actor2);
  mantarenderer->SetBackground(0.1, 0.2, 0.4);

  vtkMantaCamera *camera = vtkMantaCamera::New();
/*
  mantarenderer->SetActiveCamera(camera);
  mantarenderer->ResetCamera();
*/

  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->SetSize(480, 480);

  renWin->SetNumberOfLayers(2);

  renWin->AddRenderer(mantarenderer);
  mantarenderer->SetLayer(0);
  mantarenderer->SetViewport(0,0,0.5,0.5);

  renWin->AddRenderer(glrenderer);
  glrenderer->SetLayer(1);
  glrenderer->SetViewport(0.1,0.1,1.0,1.0);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  renWin->Render();

  iren->Start();

  source1->Delete();
  source2->Delete();
  mapper1->Delete();
  mapper2->Delete();
  actor1->Delete();
  actor2->Delete();
  camera->Delete();
  glrenderer->Delete();
  mantarenderer->Delete();
  renWin->Delete();
  iren->Delete();

  return 0;
}
