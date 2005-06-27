#include "vtkActor.h"
#include "vtkInteractorStyleSwitch.h"
#include "vtkKWApplication.h"
#include "vtkKWGenericRenderWindowInteractor.h"
#include "vtkKWRenderWidget.h"
#include "vtkKWWindow.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkXMLPolyDataReader.h"

#include "KWWidgetsTourExampleTypes.h"

class vtkKWRenderWidgetItem : public KWWidgetsTourItem
{
public:
  virtual int GetType() { return KWWidgetsTourItem::TypeVTK; };
};

KWWidgetsTourItem* vtkKWRenderWidgetEntryPoint(
  vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // Create a render widget

  vtkKWRenderWidget *rw = vtkKWRenderWidget::New();
  rw->SetParent(parent);
  rw->Create(app);

  app->Script("pack %s -side top -fill both -expand y -padx 0 -pady 0", 
              rw->GetWidgetName());

  // -----------------------------------------------------------------------

  // Switch to trackball style, it's nicer

  vtkInteractorStyleSwitch *istyle = vtkInteractorStyleSwitch::SafeDownCast(
    rw->GetRenderWindow()->GetInteractor()->GetInteractorStyle());
  if (istyle)
    {
    istyle->SetCurrentStyleToTrackballCamera();
    }

  // Create a 3D object reader

  vtkXMLPolyDataReader *reader = vtkXMLPolyDataReader::New();
  reader->SetFileName(
    KWWidgetsTourItem::GetPathToExampleData(app, "teapot.vtp"));

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

  return new vtkKWRenderWidgetItem;
}
