#include "vtkKWMenuButton.h"
#include "vtkKWMenuButtonLabeled.h"
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

  // Create another menu button, this time with a label

  vtkKWMenuButtonLabeled *menubutton2 = vtkKWMenuButtonLabeled::New();
  menubutton2->SetParent(parent);
  menubutton2->Create(app);
  menubutton2->SetBorderWidth(2);
  menubutton2->SetReliefToGroove();
  menubutton2->SetLabelText("Days:");
  menubutton2->SetPadX(2);
  menubutton2->SetPadY(2);
  menubutton2->GetWidget()->IndicatorOff();
  menubutton2->GetWidget()->SetWidth(20);
  menubutton2->SetBalloonHelpString(
    "This is a vtkKWMenuButtonLabeled, i.e. a menu button associated to a "
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
  menubutton2->Delete();

  // TODO: use callbacks

  return new vtkKWMenuButtonItem;
}
