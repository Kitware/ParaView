#include "vtkKWEntry.h"
#include "vtkKWEntryWithLabel.h"
#include "vtkKWEntrySet.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"

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
  entry1->Create();
  entry1->SetBalloonHelpString("A simple entry");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    entry1->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create another entry, but larger, and read-only

  vtkKWEntry *entry2 = vtkKWEntry::New();
  entry2->SetParent(parent);
  entry2->Create();
  entry2->SetWidth(20);
  entry2->ReadOnlyOn();
  entry2->SetValue("read-only entry");
  entry2->SetBalloonHelpString("Another entry, larger and read-only");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    entry2->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create another entry, with a label this time

  vtkKWEntryWithLabel *entry3 = vtkKWEntryWithLabel::New();
  entry3->SetParent(parent);
  entry3->Create();
  entry3->SetLabelText("Another entry, with a label in front:");
  entry3->SetBalloonHelpString(
    "This is a vtkKWEntryWithLabel, i.e. a entry associated to a "
    "label that can be positioned around the entry.");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    entry3->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create a set of entry
  // An easy way to create a bunch of related widgets without allocating
  // them one by one

  vtkKWEntrySet *entry_set = vtkKWEntrySet::New();
  entry_set->SetParent(parent);
  entry_set->Create();
  entry_set->SetBorderWidth(2);
  entry_set->SetReliefToGroove();
  entry_set->SetWidgetsPadX(1);
  entry_set->SetWidgetsPadY(1);
  entry_set->SetPadX(1);
  entry_set->SetPadY(1);
  entry_set->SetMaximumNumberOfWidgetsInPackingDirection(2);

  for (int id = 0; id < 4; id++)
    {
    vtkKWEntry *entry = entry_set->AddWidget(id);
    entry->SetBalloonHelpString(
      "This entry is part of a unique set (a vtkKWEntrySet), "
      "which provides an easy way to create a bunch of related widgets "
      "without allocating them one by one. The widgets can be layout as a "
      "NxM grid.");
    }

  // Let's be creative. The first one sets the value of the third one
  
  entry_set->GetWidget(0)->SetValue("Enter a value here...");
  entry_set->GetWidget(2)->SetValue("...and it will show there.");

  char buffer[100];
  sprintf(buffer, "SetValue [%s GetValue]", 
          entry_set->GetWidget(0)->GetTclName());
  entry_set->GetWidget(0)->SetCommand(entry_set->GetWidget(2), buffer);

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    entry_set->GetWidgetName());

  entry1->Delete();
  entry2->Delete();
  entry3->Delete();
  entry_set->Delete();

  return new vtkKWEntryItem;
}
