#include "vtkActor.h"
#include "vtkInteractorStyleSwitch.h"
#include "vtkKWApplication.h"
#include "vtkKWGenericRenderWindowInteractor.h"
#include "vtkKWRenderWidget.h"
#include "vtkKWWindow.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkXMLPolyDataReader.h"

#include <vtksys/SystemTools.hxx>

int vtkKWRenderWidgetEntryPoint(vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // Create a render widget

  vtkKWRenderWidget *rw = vtkKWRenderWidget::New();
  rw->SetParent(parent);
  rw->Create(app, NULL);

  app->Script("pack %s -side top -fill both -expand y -padx 0 -pady 0", 
              rw->GetWidgetName());

  // Switch to trackball style, it's nicer

  vtkInteractorStyleSwitch *istyle = vtkInteractorStyleSwitch::SafeDownCast(
    rw->GetRenderWindow()->GetInteractor()->GetInteractorStyle());
  if (istyle)
    {
    istyle->SetCurrentStyleToTrackballCamera();
    }

  // Create a 3D object reader

  vtkXMLPolyDataReader *reader = vtkXMLPolyDataReader::New();

  char data_path[2048];
  sprintf(data_path, "%s/Examples/Data/teapot.vtp", KWWIDGETS_SOURCE_DIR);
  if (!vtksys::SystemTools::FileExists(data_path))
    {
    sprintf(data_path, 
            "%s/../share/%s/Examples/Data/teapot.vtp",
            app->GetInstallationDirectory(), KWWIDGETS_PROJECT_NAME);
    }
  reader->SetFileName(data_path);

  // Create the mapper and actor

  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(reader->GetOutputPort());

  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);

  // Add the actor to the scene

  rw->AddProp(actor);
  rw->ResetCamera();

  reader->Delete();
  actor->Delete();
  mapper->Delete();
  rw->Delete();

  return 1;
}
