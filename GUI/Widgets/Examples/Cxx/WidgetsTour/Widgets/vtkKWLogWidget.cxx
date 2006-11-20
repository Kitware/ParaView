#include "vtkKWLogDialog.h"
#include "vtkKWLogWidget.h"
#include "vtkKWApplication.h"

#include "vtkKWWidgetsTourExample.h"

class vtkKWLogWidgetItem : public KWWidgetsTourItem
{
public:
  virtual int GetType();
  virtual void Create(vtkKWWidget *parent, vtkKWWindow *);
};

void vtkKWLogWidgetItem::Create(vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // Create a log widget

  vtkKWLogWidget *log_widget = vtkKWLogWidget::New();
  log_widget->SetParent(parent);
  log_widget->Create();
  log_widget->SetReliefToGroove();
  log_widget->SetBorderWidth(2);
  log_widget->SetPadX(2);
  log_widget->SetPadY(2);

  app->Script(
    "pack %s -side top -anchor nw -expand y -padx 2 -pady 2 -fill x", 
    log_widget->GetWidgetName());

  log_widget->Delete();

  char description[1024];
  for (int i = 0; i < 3; i++)
    {
    sprintf(description, "Error %d\nYes, I swear!\nHuge error!", i);
    log_widget->AddErrorRecord(description);
    sprintf(description, "Warning %d", i);
    log_widget->AddWarningRecord(description);
    sprintf(description, "Information %d", i);
    log_widget->AddInformationRecord(description);
    sprintf(description, "Debug %d", i);
    log_widget->AddDebugRecord(description);
    }
}

int vtkKWLogWidgetItem::GetType()
{
  return KWWidgetsTourItem::TypeComposite;
}

KWWidgetsTourItem* vtkKWLogWidgetEntryPoint()
{
  return new vtkKWLogWidgetItem();
}
