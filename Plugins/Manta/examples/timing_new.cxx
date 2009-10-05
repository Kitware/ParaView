#include <stdio.h>
#include <ostream>

#include "vtkCompositeRenderManager.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDistributedDataFilter.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkPieceScalars.h"
#include "vtkMPIController.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataReader.h"
#include "vtkMPICommunicator.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkIdTypeArray.h"
#include "vtkTriangle.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkCamera.h"
#include "vtkTimerLog.h"
#include "vtkProperty.h"
#include "vtkLight.h"
#include "vtkTriangleFilter.h"
#include "vtkTransmitPolyDataPiece.h"
#include "vtkPolyData.h"
#include "vtkSelection.h"
#include "vtkExtractSelectedPolyDataIds.h"
#include "vtkInformation.h"
#include "vtkLoopSubdivisionFilter.h"
#include "vtkRTAnalyticSource.h"
#include "vtkImageData.h"
#include "vtkContourFilter.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkCommand.h"
#include "vtkLookupTable.h"
#include "vtkPlaneSource.h"

// Callback for the interaction
class vtkMyCallback : public vtkCommand {
public:
    char title[50];
    vtkTimerLog* timer;
    char *old_title;
    double total_time;

    vtkMyCallback()
    {
      this->old_title = 0;
      this->timer  = vtkTimerLog::New();
      this->total_time = 0.0;
    }
    ~vtkMyCallback()
    {
      this->timer->Delete();
    }

    static vtkMyCallback *New()
    {
      return new vtkMyCallback;
    }

    virtual void Execute(vtkObject *caller, unsigned long id, void*)
    {
      vtkRenderWindow *renWin = reinterpret_cast<vtkRenderWindow*>(caller);
      if (!this->old_title) {
        this->old_title = strdup(renWin->GetWindowName());
      }
      switch (id)
      {
        case vtkCommand::StartEvent:
          this->timer->StartTimer();
          break;
        case vtkCommand::EndEvent:
          timer->StopTimer();
          //sprintf(title, "%s fps %4.2f", this->old_title, 1.0/(timer->GetElapsedTime()));
          //renWin->SetWindowName(title);
          fprintf(stderr, "render time: %f\n", timer->GetElapsedTime());
          break;
      }
    }
};

double slate_grey[] = { 0.4392, 0.5020, 0.5647 };

void addLights(vtkRenderer* renderer)
{
  /* problem with shadow, the following combination cause near infinite loop
   * 1. hard(-attenuateShadows)
   * 2. vtkLight *light1 = vtkLight::New();
   *    //light1->PositionalOn();
   *    //light1->SetPosition(renderer->GetActiveCamera()->GetPosition());
   *    //light1->SetFocalPoint(renderer->GetActiveCamera()->GetFocalPoint());
   *    light1->SetColor(0.5, 0.5, 0.5);
   *    //light1->SetLightTypeToCameraLight();
   */
  // light in front of camera
  vtkLight *light1 = vtkLight::New();
  light1->PositionalOn();
  light1->SetPosition(renderer->GetActiveCamera()->GetPosition());
  light1->SetFocalPoint(renderer->GetActiveCamera()->GetFocalPoint());
  light1->SetColor(0.5, 0.5, 0.5);
  light1->SetLightTypeToCameraLight();
  renderer->AddLight(light1);
  light1->Delete();

  // light in upper right
  vtkLight *light2 = vtkLight::New();
  light2->PositionalOn();
  light2->SetPosition(3, 3, 3);
  light2->SetColor(0.7, 0.7, 0.7);
  light2->SetLightTypeToCameraLight();
  renderer->AddLight(light2);
  light2->Delete();
}

double render(vtkRenderWindow* renWin)
{
  vtkTimerLog* timer = vtkTimerLog::New();

  timer->StartTimer();
  renWin->Render();
  timer->StopTimer();

  //if (myId == 0)
  //  {
    fprintf(stderr, "render time: %f\n", timer->GetElapsedTime());
  //  }

  timer->Delete();
  //return timer->GetElapsedTime();
  return 0.0;
}

int main(int argc, char *argv[])
{
  vtkPolyDataReader* meshReader = vtkPolyDataReader::New();
  meshReader->SetFileName(argv[1]);

  vtkLookupTable *lut = vtkLookupTable::New();
  lut->Build();

  vtkPolyDataMapper* meshMapper = vtkPolyDataMapper::New();
  meshMapper->SetInputConnection(meshReader->GetOutputPort());
  meshMapper->SetScalarModeToUsePointFieldData();
  meshMapper->SetScalarRange(-1, 1);
  meshMapper->ColorByArrayComponent("Normals", 0);
  meshMapper->SetLookupTable(lut);

  vtkActor* meshActor = vtkActor::New();
  meshActor->SetMapper(meshMapper);

  double diffuseColor[3] = { 0.73, 0.9383, 0.2739 };
  vtkProperty* property = (vtkProperty*) meshActor->GetProperty();
  property->SetColor(diffuseColor);
  property->SetSpecularColor(0.5, 0.5, 0.5);
  property->SetSpecular(0.2);
  property->SetSpecularPower(100);

  vtkPlaneSource *planeSource = vtkPlaneSource::New();
  planeSource->SetOrigin(-2, -2, -1);
  planeSource->SetPoint1(2, -2, -1);
  planeSource->SetPoint2(-2, 2, -1);
  planeSource->SetXResolution(1);
  planeSource->SetYResolution(1);

  vtkPolyDataMapper *planeMapper = vtkPolyDataMapper::New();
  planeMapper->SetInputConnection(planeSource->GetOutputPort());
  vtkActor* planeActor = vtkActor::New();
  planeActor->SetMapper(planeMapper);

  vtkRenderer* renderer = vtkRenderer::New();
  renderer->SetBackground(slate_grey);
  renderer->AddActor(meshActor);
  renderer->AddActor(planeActor);

  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->AddRenderer(renderer);
  renWin->SetSize(640, 480);

  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // Here is where we setup the observer, we do a new and ren1 will
  // eventually free the observer
  vtkMyCallback *mo1 = vtkMyCallback::New();
  renWin->AddObserver(vtkCommand::StartEvent, mo1);
  renWin->AddObserver(vtkCommand::EndEvent, mo1);
  mo1->Delete();

  //renderer->ResetCamera();
  vtkCamera *camera = renderer->GetActiveCamera();
  camera->SetPosition(3, 0, 2);
  camera->SetFocalPoint(0, 0, 0);
  camera->SetViewUp(0, 0, 1);

  addLights(renderer);

  renWin->Render();

  //  getchar();
  for (int i = 0; i <= 360; ++i)
  //for (;;)
    {
    // rotate the active camera by one degree
    renderer->GetActiveCamera()->Azimuth( 1);
    render(renWin);
  }


  iren->Start();

  renWin->Delete();
}
