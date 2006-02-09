#include "vtkKWComboBox.h"
#include "vtkKWComboBoxWithLabel.h"
#include "vtkKWComboBoxSet.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"

#include "vtkKWWidgetsTourExample.h"

class vtkKWComboBoxItem : public KWWidgetsTourItem
{
public:
  virtual int GetType();
  virtual void Create(vtkKWWidget *parent, vtkKWWindow *win);
};

void vtkKWComboBoxItem::Create(vtkKWWidget *parent, vtkKWWindow *win)
{
  vtkKWApplication *app = parent->GetApplication();

  size_t i;
  const char* days[] = 
    {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};
  const char* numbers[] = 
    {"123", "42", "007", "1789"};

  // -----------------------------------------------------------------------

  // Create a combobox

  vtkKWComboBox *combobox1 = vtkKWComboBox::New();
  combobox1->SetParent(parent);
  combobox1->Create();
  combobox1->SetBalloonHelpString("A simple combobox");

  for (i = 0; i < sizeof(days) / sizeof(days[0]); i++)
    {
    combobox1->AddValue(days[i]);
    }

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    combobox1->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create another combobox, but larger, and read-only

  vtkKWComboBox *combobox2 = vtkKWComboBox::New();
  combobox2->SetParent(parent);
  combobox2->Create();
  combobox2->SetWidth(20);
  combobox2->ReadOnlyOn();
  combobox2->SetValue("read-only combobox");
  combobox2->SetBalloonHelpString("Another combobox, larger and read-only");

  for (i = 0; i < sizeof(numbers) / sizeof(numbers[0]); i++)
    {
    combobox2->AddValue(numbers[i]);
    }

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    combobox2->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create another combobox, with a label this time

  vtkKWComboBoxWithLabel *combobox3 = vtkKWComboBoxWithLabel::New();
  combobox3->SetParent(parent);
  combobox3->Create();
  combobox3->SetLabelText("Another combobox, with a label in front:");
  combobox3->SetBalloonHelpString(
    "This is a vtkKWComboBoxWithLabel, i.e. a combobox associated to a "
    "label that can be positioned around the combobox.");

  for (i = 0; i < sizeof(days) / sizeof(days[0]); i++)
    {
    combobox3->GetWidget()->AddValue(days[i]);
    }

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    combobox3->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create a set of combobox
  // An easy way to create a bunch of related widgets without allocating
  // them one by one

  vtkKWComboBoxSet *combobox_set = vtkKWComboBoxSet::New();
  combobox_set->SetParent(parent);
  combobox_set->Create();
  combobox_set->SetBorderWidth(2);
  combobox_set->SetReliefToGroove();
  combobox_set->SetWidgetsPadX(1);
  combobox_set->SetWidgetsPadY(1);
  combobox_set->SetPadX(1);
  combobox_set->SetPadY(1);
  combobox_set->SetMaximumNumberOfWidgetsInPackingDirection(2);

  for (int id = 0; id < 4; id++)
    {
    vtkKWComboBox *combobox = combobox_set->AddWidget(id);
    combobox->SetBalloonHelpString(
      "This combobox is part of a unique set (a vtkKWComboBoxSet), "
      "which provides an easy way to create a bunch of related widgets "
      "without allocating them one by one. The widgets can be layout as a "
      "NxM grid.");

    for (i = 0; i < sizeof(days) / sizeof(days[0]); i++)
      {
      combobox->AddValue(days[i]);
      }
    }

  // Let's be creative. The first one sets the value of the third one
  
  combobox_set->GetWidget(0)->SetValue("Enter a value here...");
  combobox_set->GetWidget(2)->SetValue("...and it will show here.");
  combobox_set->GetWidget(2)->DeleteAllValues();

  combobox_set->GetWidget(0)->SetCommand(
    combobox_set->GetWidget(2), "SetValue");

  // Let's be creative. The second one adds its value to the fourth one
  
  combobox_set->GetWidget(1)->SetValue("Enter a value here...");
  combobox_set->GetWidget(3)->SetValue("...and it will be added here.");
  combobox_set->GetWidget(3)->DeleteAllValues();

  combobox_set->GetWidget(1)->SetCommand(
    combobox_set->GetWidget(3), "AddValue");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    combobox_set->GetWidgetName());

  combobox1->Delete();
  combobox2->Delete();
  combobox3->Delete();
  combobox_set->Delete();
}

int vtkKWComboBoxItem::GetType()
{
  return KWWidgetsTourItem::TypeCore;
}

KWWidgetsTourItem* vtkKWComboBoxEntryPoint()
{
  return new vtkKWComboBoxItem();
}
