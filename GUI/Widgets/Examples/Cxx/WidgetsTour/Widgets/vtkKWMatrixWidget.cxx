#include "vtkKWMatrixWidget.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"

#include "vtkKWWidgetsTourExample.h"

class vtkKWMatrixWidgetItem : public KWWidgetsTourItem
{
public:
  virtual int GetType();
  virtual void Create(vtkKWWidget *parent, vtkKWWindow *);
};

void vtkKWMatrixWidgetItem::Create(vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // Create a matrix widget

  vtkKWMatrixWidget *matrix_widget1 = vtkKWMatrixWidget::New();
  matrix_widget1->SetParent(parent);
  matrix_widget1->Create();
  matrix_widget1->SetBorderWidth(2);
  matrix_widget1->SetReliefToGroove();
  matrix_widget1->SetPadX(2);
  matrix_widget1->SetPadY(2);

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    matrix_widget1->GetWidgetName());

  matrix_widget1->Delete();
}

int vtkKWMatrixWidgetItem::GetType()
{
  return KWWidgetsTourItem::TypeComposite;
}

KWWidgetsTourItem* vtkKWMatrixWidgetEntryPoint()
{
  return new vtkKWMatrixWidgetItem();
}
