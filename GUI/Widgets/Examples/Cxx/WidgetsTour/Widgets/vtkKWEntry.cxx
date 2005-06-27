#include "vtkKWEntry.h"
#include "vtkKWEntryLabeled.h"
#include "vtkKWEntrySet.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"
#include "vtkKWIcon.h"

#include <vtksys/stl/string>
#include "KWWidgetsTourExampleTypes.h"

class vtkKWEntryItem : public KWWidgetsTourItem
{
public:
  virtual int GetType() { return KWWidgetsTourItem::TypeCore; };
};

KWWidgetsTourItem* vtkKWEntryEntryPoint(
  vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // Create a entry

  vtkKWEntry *entry1 = vtkKWEntry::New();
  entry1->SetParent(parent);
  entry1->Create(app);
  entry1->SetBalloonHelpString("A simple entry");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    entry1->GetWidgetName());

  entry1->Delete();

  // -----------------------------------------------------------------------

  // Create another entry, but larger, and read-only

  vtkKWEntry *entry2 = vtkKWEntry::New();
  entry2->SetParent(parent);
  entry2->Create(app);
  entry2->SetWidth(20);
  entry2->ReadOnlyOn();
  entry2->SetValue("read-only entry");
  entry2->SetBalloonHelpString("Another entry, larger and read-only");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    entry2->GetWidgetName());

  entry2->Delete();

  // -----------------------------------------------------------------------

  // Create another entry, with a label this time

  vtkKWEntryLabeled *entry3 = vtkKWEntryLabeled::New();
  entry3->SetParent(parent);
  entry3->Create(app);
  entry3->SetLabelText("Another entry, with a label in front:");
  entry3->SetBalloonHelpString(
    "This is a vtkKWEntryLabeled, i.e. a entry associated to a "
    "label that can be positioned around the entry.");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    entry3->GetWidgetName());

  entry3->Delete();

  // -----------------------------------------------------------------------

  // Create a set of entry
  // An easy way to create a bunch of related widgets without allocating
  // them one by one

  vtkKWEntrySet *entry_set = vtkKWEntrySet::New();
  entry_set->SetParent(parent);
  entry_set->Create(app);
  entry_set->SetBorderWidth(2);
  entry_set->SetReliefToGroove();
  entry_set->SetWidgetsPadX(2);
  entry_set->SetWidgetsPadY(2);

  for (int i = 0; i < 4; i++)
    {
    vtkKWEntry *entry = entry_set->AddWidget(i);
    entry->SetBalloonHelpString(
      "This entry is part of a unique set (a vtkKWEntrySet), "
      "which provides an easy way to create a bunch of related widgets "
      "without allocating them one by one.");
    }

  // Let's be creative. The first one also set the value of the last one
  
  entry_set->GetWidget(0)->SetValue("Enter a value here...");
  entry_set->GetWidget(3)->SetValue("...and it will show there.");

  char buffer[100];
  sprintf(buffer, "SetValue [%s GetValue]", 
          entry_set->GetWidget(0)->GetTclName());
  entry_set->GetWidget(0)->BindCommand(entry_set->GetWidget(3), buffer);

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    entry_set->GetWidgetName());

  entry_set->Delete();

  return new vtkKWEntryItem;
}
