#include "vtkKWLoadSaveButton.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"

#include "vtkKWWidgetsTourExample.h"

class vtkKWLoadSaveButtonItem : public KWWidgetsTourItem
{
public:
  virtual int GetType();
  virtual void Create(vtkKWWidget *parent, vtkKWWindow *);
};

void vtkKWLoadSaveButtonItem::Create(vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // Create a load button

  vtkKWLoadSaveButton *load_button1 = vtkKWLoadSaveButton::New();
  load_button1->SetParent(parent);
  load_button1->Create();
  load_button1->SetText("Click to Pick a File");
  load_button1->GetLoadSaveDialog()->SaveDialogOff(); // load mode

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    load_button1->GetWidgetName());

  load_button1->Delete();
}

int vtkKWLoadSaveButtonItem::GetType()
{
  return KWWidgetsTourItem::TypeComposite;
}

KWWidgetsTourItem* vtkKWLoadSaveButtonEntryPoint()
{
  return new vtkKWLoadSaveButtonItem();
}
