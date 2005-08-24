#include "vtkKWMenuButton.h"
#include "vtkKWMenuButtonWithSpinButtons.h"
#include "vtkKWMenuButtonWithLabel.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"

#include "KWWidgetsTourExampleTypes.h"

class vtkKWMenuButtonItem : public KWWidgetsTourItem
{
public:
  virtual int GetType() { return KWWidgetsTourItem::TypeCore; };
};

KWWidgetsTourItem* vtkKWMenuButtonEntryPoint(vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  size_t i;
  const char* days[] = 
    {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};

  // -----------------------------------------------------------------------

  // Create a menu button
  // Add some entries

  vtkKWMenuButton *menubutton1 = vtkKWMenuButton::New();
  menubutton1->SetParent(parent);
  menubutton1->Create(app);
  menubutton1->SetBalloonHelpString("A simple menu button");

  for (i = 0; i < sizeof(days) / sizeof(days[0]); i++)
    {
    menubutton1->AddRadioButton(days[i]);
    }

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    menubutton1->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create a menu button with spin buttons

  vtkKWMenuButtonWithSpinButtons *menubutton1b = vtkKWMenuButtonWithSpinButtons::New();
  menubutton1b->SetParent(parent);
  menubutton1b->Create(app);
  menubutton1b->GetWidget()->SetWidth(20);
  menubutton1b->SetBalloonHelpString(
    "This is a vtkKWMenuButtonWithSpinButtons, i.e. a menu button associated "
    "to a set of spin buttons (vtkKWSpinButtons) that can be used to "
    "increment and decrement the value");

  for (i = 0; i < sizeof(days) / sizeof(days[0]); i++)
    {
    menubutton1b->GetWidget()->AddRadioButton(days[i]);
    }

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    menubutton1b->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create another menu button, this time with a label

  vtkKWMenuButtonWithLabel *menubutton2 = vtkKWMenuButtonWithLabel::New();
  menubutton2->SetParent(parent);
  menubutton2->Create(app);
  menubutton2->SetBorderWidth(2);
  menubutton2->SetReliefToGroove();
  menubutton2->SetLabelText("Days:");
  menubutton2->SetPadX(2);
  menubutton2->SetPadY(2);
  menubutton2->GetWidget()->IndicatorVisibilityOff();
  menubutton2->GetWidget()->SetWidth(20);
  menubutton2->SetBalloonHelpString(
    "This is a vtkKWMenuButtonWithLabel, i.e. a menu button associated to a "
    "label that can be positioned around the menu button. The indicator is "
    "hidden, and the width is set explicitly");

  for (i = 0; i < sizeof(days) / sizeof(days[0]); i++)
    {
    menubutton2->GetWidget()->AddRadioButton(days[i]);
    }

  menubutton2->GetWidget()->SetValue(days[0]);

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    menubutton2->GetWidgetName());

  menubutton1->Delete();
  menubutton1b->Delete();
  menubutton2->Delete();

  // TODO: use callbacks

  return new vtkKWMenuButtonItem;
}
