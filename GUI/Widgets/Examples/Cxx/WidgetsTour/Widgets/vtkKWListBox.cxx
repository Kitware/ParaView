#include "vtkKWListBox.h"
#include "vtkKWListBoxWithScrollbars.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"

#include "KWWidgetsTourExampleTypes.h"

class vtkKWListBoxItem : public KWWidgetsTourItem
{
public:
  virtual int GetType() { return KWWidgetsTourItem::TypeCore; };
};

KWWidgetsTourItem* vtkKWListBoxEntryPoint(vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  size_t i;
  const char* days[] = 
    {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};

  // -----------------------------------------------------------------------

  // Create a listbox
  // Add some entries

  vtkKWListBox *listbox1 = vtkKWListBox::New();
  listbox1->SetParent(parent);
  listbox1->Create();
  listbox1->SetSelectionModeToSingle();
  listbox1->SetBalloonHelpString("A simple listbox");

  for (i = 0; i < sizeof(days) / sizeof(days[0]); i++)
    {
    listbox1->AppendUnique(days[i]);
    }

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    listbox1->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create another listbox, this time with scrollbars and multiple choices

  vtkKWListBoxWithScrollbars *listbox2 = vtkKWListBoxWithScrollbars::New();
  listbox2->SetParent(parent);
  listbox2->Create();
  listbox2->SetBorderWidth(2);
  listbox2->SetReliefToGroove();
  listbox2->SetPadX(2);
  listbox2->SetPadY(2);
  listbox2->GetWidget()->SetSelectionModeToMultiple();
  listbox2->GetWidget()->ExportSelectionOff();
  listbox2->SetBalloonHelpString(
    "A list box with scrollbars. Multiple entries can be selected");

  char buffer[20];
  for (i = 0; i < 15; i++)
    {
    sprintf(buffer, "Entry %d", (int)i);
    listbox2->GetWidget()->AppendUnique(buffer);
    }

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    listbox2->GetWidgetName());

  listbox1->Delete();
  listbox2->Delete();

  // TODO: use callbacks

  return new vtkKWListBoxItem;
}
