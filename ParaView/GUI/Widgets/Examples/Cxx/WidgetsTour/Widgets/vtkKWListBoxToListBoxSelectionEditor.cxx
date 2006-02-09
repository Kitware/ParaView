#include "vtkKWListBoxToListBoxSelectionEditor.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"

#include "vtkKWWidgetsTourExample.h"

class vtkKWListBoxToListBoxSelectionEditorItem : public KWWidgetsTourItem
{
public:
  virtual int GetType();
  virtual void Create(vtkKWWidget *parent, vtkKWWindow *win);
};

void vtkKWListBoxToListBoxSelectionEditorItem::Create(vtkKWWidget *parent, vtkKWWindow *win)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // Create a list box to list box selection editor

  vtkKWListBoxToListBoxSelectionEditor *lb2lb1 = vtkKWListBoxToListBoxSelectionEditor::New();
  lb2lb1->SetParent(parent);
  lb2lb1->Create();
  lb2lb1->SetReliefToGroove();
  lb2lb1->SetBorderWidth(2);
  lb2lb1->SetPadX(2);
  lb2lb1->SetPadY(2);

  lb2lb1->AddSourceElement("Monday", 0);
  lb2lb1->AddSourceElement("Tuesday", 0);
  lb2lb1->AddSourceElement("Wednesday", 0);
  lb2lb1->AddFinalElement("Thursday", 0);
  lb2lb1->AddFinalElement("Friday", 0);

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    lb2lb1->GetWidgetName());

  lb2lb1->Delete();
}

int vtkKWListBoxToListBoxSelectionEditorItem::GetType()
{
  return KWWidgetsTourItem::TypeComposite;
}

KWWidgetsTourItem* vtkKWListBoxToListBoxSelectionEditorEntryPoint()
{
  return new vtkKWListBoxToListBoxSelectionEditorItem();
}
