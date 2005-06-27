#include "vtkKWRadioButton.h"
#include "vtkKWRadioButton.h"
#include "vtkKWRadioButtonSet.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"
#include "vtkKWIcon.h"

#include <vtksys/stl/string>
#include "KWWidgetsTourExampleTypes.h"

class vtkKWRadioButtonItem : public KWWidgetsTourItem
{
public:
  virtual int GetType() { return KWWidgetsTourItem::TypeCore; };
};

KWWidgetsTourItem* vtkKWRadioButtonEntryPoint(
  vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // Create two radiobuttons. 
  // They share the same variable name and each one has a different internal
  // value.

  vtkKWRadioButton *radiob1 = vtkKWRadioButton::New();
  radiob1->SetParent(parent);
  radiob1->Create(app);
  radiob1->SetText("A radiobutton");
  radiob1->SetValue(123);

  vtkKWRadioButton *radiob1b = vtkKWRadioButton::New();
  radiob1b->SetParent(parent);
  radiob1b->Create(app);
  radiob1b->SetText("Another radiobutton");
  radiob1b->SetValue(456);

  radiob1->SetState(1);
  radiob1b->SetVariableName(radiob1->GetVariableName());

  app->Script(
    "pack %s %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    radiob1->GetWidgetName(), radiob1b->GetWidgetName());

  radiob1->Delete();
  radiob1b->Delete();

  // -----------------------------------------------------------------------

  // Create two radiobuttons. Use icons
  // They share the same variable name and each one has a different internal
  // value.

  vtkKWRadioButton *radiob2 = vtkKWRadioButton::New();
  radiob2->SetParent(parent);
  radiob2->Create(app);
  radiob2->SetImageOption(vtkKWIcon::IconPlus);
  radiob2->IndicatorOff();
  radiob2->SetValue("foo");

  vtkKWRadioButton *radiob2b = vtkKWRadioButton::New();
  radiob2b->SetParent(parent);
  radiob2b->Create(app);
  radiob2b->SetImageOption(vtkKWIcon::IconMinus);
  radiob2b->IndicatorOff();
  radiob2b->SetValue("bar");

  radiob2->SetState(1);
  radiob2b->SetVariableName(radiob2->GetVariableName());

  app->Script(
    "pack %s %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    radiob2->GetWidgetName(), radiob2b->GetWidgetName());

  radiob2->Delete();
  radiob2b->Delete();

  // -----------------------------------------------------------------------

  // Create a set of radiobutton
  // An easy way to create a bunch of related widgets without allocating
  // them one by one

  vtkKWRadioButtonSet *radiob_set = vtkKWRadioButtonSet::New();
  radiob_set->SetParent(parent);
  radiob_set->Create(app);
  radiob_set->SetBorderWidth(2);
  radiob_set->SetReliefToGroove();

  char buffer[50];
  for (int i = 0; i < 4; i++)
    {
    sprintf(buffer, "Radiobutton %d", i);
    vtkKWRadioButton *radiob = radiob_set->AddWidget(i);
    radiob->SetText(buffer);
    radiob->SetBalloonHelpString(
      "This radiobutton is part of a unique set (a vtkKWRadioButtonSet), "
      "which provides an easy way to create a bunch of related widgets "
      "without allocating them one by one. The widgets can be layout as a "
      "NxM grid. This classes automatically set the same variable name "
      "among all radiobuttons, as well as a unique value.");
    }
  
  radiob_set->GetWidget(0)->SetState(1);

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    radiob_set->GetWidgetName());

  radiob_set->Delete();

  // -----------------------------------------------------------------------

  // TODO: use vtkKWRadioButtonSetLabeled and callbacks

  return new vtkKWRadioButtonItem;
}
