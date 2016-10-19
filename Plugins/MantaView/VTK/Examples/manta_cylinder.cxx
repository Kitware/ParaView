/*
Example of rendering something with vtkManta.
*/

#include "vtkCylinderSource.h"
#include "vtkMantaActor.h"
#include "vtkMantaCamera.h"
#include "vtkMantaPolyDataMapper.h"
#include "vtkMantaProperty.h"
#include "vtkMantaRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

double lamp_black[] = { 0.1800, 0.2800, 0.2300 };

int main()
{
  vtkCylinderSource* source2 = vtkCylinderSource::New();
  source2->SetHeight(3.0);
  source2->SetRadius(1.0);
  source2->SetResolution(5);

  vtkMantaPolyDataMapper* mapper2 = vtkMantaPolyDataMapper::New();
  mapper2->SetInputConnection(source2->GetOutputPort());

  vtkMantaActor* actor2 = vtkMantaActor::New();
  actor2->SetMapper(mapper2);

  vtkProperty* property2 = actor2->GetProperty();
  property2->SetColor(1.0, 0.3882, 0.2784);
  property2->SetDiffuse(0.7);
  property2->SetSpecular(0.4);
  property2->SetSpecularPower(20);
  property2->SetInterpolationToFlat();
  actor2->SetProperty(property2);

  vtkMantaRenderer* mantarenderer = vtkMantaRenderer::New();
  mantarenderer->AddActor(actor2);
  mantarenderer->SetBackground(0.1, 0.2, 0.4);

  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->SetSize(480, 480);

  renWin->AddRenderer(mantarenderer);

  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  renWin->Render();
  iren->Start();

  source2->Delete();
  mapper2->Delete();
  actor2->Delete();
  mantarenderer->Delete();
  renWin->Delete();
  iren->Delete();

  return 0;
}
