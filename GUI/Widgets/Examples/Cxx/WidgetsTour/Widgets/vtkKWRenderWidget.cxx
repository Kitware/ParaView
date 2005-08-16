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

  vtkKWRenderWidget *rw_renderwidget = vtkKWRenderWidget::New();
  rw_renderwidget->SetParent(parent);
  rw_renderwidget->Create(app);

  app->Script("pack %s -side top -fill both -expand y -padx 0 -pady 0", 
              rw_renderwidget->GetWidgetName());

  // -----------------------------------------------------------------------

  // Switch to trackball style, it's nicer

  vtkInteractorStyleSwitch *istyle = vtkInteractorStyleSwitch::SafeDownCast(
    rw_renderwidget->GetRenderWindow()->GetInteractor()->GetInteractorStyle());
  if (istyle)
    {
    istyle->SetCurrentStyleToTrackballCamera();
    }

  // Create a 3D object reader

  vtkXMLPolyDataReader *rw_reader = vtkXMLPolyDataReader::New();
  rw_reader->SetFileName(
    KWWidgetsTourItem::GetPathToExampleData(app, "teapot.vtp"));

  // Create the mapper and actor

  vtkPolyDataMapper *rw_mapper = vtkPolyDataMapper::New();
  rw_mapper->SetInputConnection(rw_reader->GetOutputPort());

  vtkActor *rw_actor = vtkActor::New();
  rw_actor->SetMapper(rw_mapper);

  // Add the actor to the scene

  rw_renderwidget->AddViewProp(rw_actor);
  rw_renderwidget->ResetCamera();

  rw_reader->Delete();
  rw_actor->Delete();
  rw_mapper->Delete();
  rw_renderwidget->Delete();

  return new vtkKWRenderWidgetItem;
}
