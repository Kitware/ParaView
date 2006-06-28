#include "vtkKWSpinBox.h"
#include "vtkKWSpinBoxWithLabel.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"

#include "vtkKWWidgetsTourExample.h"

class vtkKWSpinBoxItem : public KWWidgetsTourItem
{
public:
  virtual int GetType();
  virtual void Create(vtkKWWidget *parent, vtkKWWindow *);
};

void vtkKWSpinBoxItem::Create(vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // Create a spinbox

  vtkKWSpinBox *spinbox1 = vtkKWSpinBox::New();
  spinbox1->SetParent(parent);
  spinbox1->Create();
  spinbox1->SetRange(0, 10);
  spinbox1->SetIncrement(1);
  spinbox1->RestrictValuesToIntegersOn();
  spinbox1->SetBalloonHelpString("A simple spinbox");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    spinbox1->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create another spinbox, but larger, and read-only

  vtkKWSpinBox *spinbox2 = vtkKWSpinBox::New();
  spinbox2->SetParent(parent);
  spinbox2->Create();
  spinbox2->SetRange(10.0, 15.0);
  spinbox2->SetIncrement(0.5);
  spinbox2->SetValue(12);
  spinbox2->SetValueFormat("%.1f");
  spinbox2->SetWidth(5);
  spinbox2->WrapOn();
  spinbox2->SetBalloonHelpString("Another spinbox, that wraps around");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    spinbox2->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create another spinbox, with a label this time

  vtkKWSpinBoxWithLabel *spinbox3 = vtkKWSpinBoxWithLabel::New();
  spinbox3->SetParent(parent);
  spinbox3->Create();
  spinbox3->GetWidget()->SetRange(10, 100);
  spinbox3->GetWidget()->SetIncrement(10);
  spinbox3->SetLabelText("Another spinbox, with a label in front:");
  spinbox3->SetBalloonHelpString(
    "This is a vtkKWSpinBoxWithLabel, i.e. a spinbox associated to a "
    "label that can be positioned around the spinbox.");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    spinbox3->GetWidgetName());

  spinbox1->Delete();
  spinbox2->Delete();
  spinbox3->Delete();
}

int vtkKWSpinBoxItem::GetType()
{
  return KWWidgetsTourItem::TypeCore;
}

KWWidgetsTourItem* vtkKWSpinBoxEntryPoint()
{
  return new vtkKWSpinBoxItem();
}
