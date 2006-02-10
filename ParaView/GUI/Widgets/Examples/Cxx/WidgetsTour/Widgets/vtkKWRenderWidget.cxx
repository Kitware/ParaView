#include "vtkActor.h"
#include "vtkKWApplication.h"
#include "vtkKWRenderWidget.h"
#include "vtkKWWindow.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkXMLPolyDataReader.h"

#include "vtkKWWidgetsTourExample.h"

class vtkKWRenderWidgetItem : public KWWidgetsTourItem
{
public:
  virtual int GetType();
  virtual void Create(vtkKWWidget *parent, vtkKWWindow *);
};

void vtkKWRenderWidgetItem::Create(vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // Create a render widget

  vtkKWRenderWidget *rw_renderwidget = vtkKWRenderWidget::New();
  rw_renderwidget->SetParent(parent);
  rw_renderwidget->Create();

  app->Script("pack %s -side top -fill both -expand y -padx 0 -pady 0", 
              rw_renderwidget->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create a 3D object reader

  vtkXMLPolyDataReader *rw_reader = vtkXMLPolyDataReader::New();
  rw_reader->SetFileName(
    vtkKWWidgetsTourExample::GetPathToExampleData(app, "teapot.vtp"));

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
}

int vtkKWRenderWidgetItem::GetType()
{
  return KWWidgetsTourItem::TypeVTK;
}

KWWidgetsTourItem* vtkKWRenderWidgetEntryPoint()
{
  return new vtkKWRenderWidgetItem();
}
