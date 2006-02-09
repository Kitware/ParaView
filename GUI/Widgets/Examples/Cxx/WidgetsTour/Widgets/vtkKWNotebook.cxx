#include "vtkKWNotebook.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"

#include "vtkKWWidgetsTourExample.h"

class vtkKWNotebookItem : public KWWidgetsTourItem
{
public:
  virtual int GetType();
  virtual void Create(vtkKWWidget *parent, vtkKWWindow *win);
};

void vtkKWNotebookItem::Create(vtkKWWidget *parent, vtkKWWindow *win)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // Create a notebook

  vtkKWNotebook *notebook1 = vtkKWNotebook::New();
  notebook1->SetParent(parent);
  notebook1->SetMinimumWidth(400);
  notebook1->SetMinimumHeight(200);
  notebook1->Create();

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    notebook1->GetWidgetName());

  // Add some pages

  notebook1->AddPage("Page 1");

  notebook1->AddPage("Page Blue");
  notebook1->GetFrame("Page Blue")->SetBackgroundColor(0.2, 0.2, 0.9);

  int page_id = notebook1->AddPage("Page Red");
  notebook1->GetFrame(page_id)->SetBackgroundColor(0.9, 0.2, 0.2);

  notebook1->Delete();
}

int vtkKWNotebookItem::GetType()
{
  return KWWidgetsTourItem::TypeComposite;
}

KWWidgetsTourItem* vtkKWNotebookEntryPoint()
{
  return new vtkKWNotebookItem();
}
