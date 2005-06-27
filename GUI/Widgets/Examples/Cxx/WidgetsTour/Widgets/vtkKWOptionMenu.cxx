#include "vtkKWOptionMenu.h"
#include "vtkKWOptionMenuLabeled.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"

#include "KWWidgetsTourExampleTypes.h"

class vtkKWOptionMenuItem : public KWWidgetsTourItem
{
public:
  virtual int GetType() { return KWWidgetsTourItem::TypeCore; };
};

KWWidgetsTourItem* vtkKWOptionMenuEntryPoint(vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  int i;
  const char* days[] = 
    {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};

  // -----------------------------------------------------------------------

  // Create a optionmenu
  // Add some entries

  vtkKWOptionMenu *optionmenu1 = vtkKWOptionMenu::New();
  optionmenu1->SetParent(parent);
  optionmenu1->Create(app);
  optionmenu1->SetBalloonHelpString("A simple option menu");

  for (i = 0; i < sizeof(days) / sizeof(days[0]); i++)
    {
    optionmenu1->AddEntry(days[i]);
    }

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    optionmenu1->GetWidgetName());

  optionmenu1->Delete();

  // -----------------------------------------------------------------------

  // Create another optionmenu, this time with a label

  vtkKWOptionMenuLabeled *optionmenu2 = vtkKWOptionMenuLabeled::New();
  optionmenu2->SetParent(parent);
  optionmenu2->Create(app);
  optionmenu2->SetBorderWidth(2);
  optionmenu2->SetReliefToGroove();
  optionmenu2->SetLabelText("Days:");
  optionmenu2->SetPadX(2);
  optionmenu2->SetPadY(2);
  optionmenu2->GetWidget()->IndicatorOff();
  optionmenu2->GetWidget()->SetWidth(20);
  optionmenu2->SetBalloonHelpString(
    "This is a vtkKWOptionMenuLabeled, i.e. an option menu associated to a "
    "label that can be positioned around the option menu. The indicator is "
    "hidden, and the width is set explicitly");

  for (i = 0; i < sizeof(days) / sizeof(days[0]); i++)
    {
    optionmenu2->GetWidget()->AddEntry(days[i]);
    }

  optionmenu2->GetWidget()->SetValue(days[0]);

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    optionmenu2->GetWidgetName());

  optionmenu2->Delete();

  // TODO: use callbacks

  return new vtkKWOptionMenuItem;
}
