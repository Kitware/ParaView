#include "vtkKWProgressGauge.h"
#include "vtkKWPushButtonSet.h"
#include "vtkKWPushButton.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"

#include <vtksys/stl/string>
#include "KWWidgetsTourExampleTypes.h"

class vtkKWProgressGaugeItem : public KWWidgetsTourItem
{
public:
  virtual int GetType() { return KWWidgetsTourItem::TypeComposite; };
};

KWWidgetsTourItem* vtkKWProgressGaugeEntryPoint(
  vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // Create a progress gauge

  vtkKWProgressGauge *progress1 = vtkKWProgressGauge::New();
  progress1->SetParent(parent);
  progress1->Create(app);
  progress1->SetWidth(150);
  progress1->SetBorderWidth(2);
  progress1->SetReliefToGroove();
  progress1->SetPadX(2);
  progress1->SetPadY(2);

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    progress1->GetWidgetName());

  progress1->Delete();

  // -----------------------------------------------------------------------

  // Create a set of pushbutton that will modify the progress gauge

  vtkKWPushButtonSet *pushbutton_set = vtkKWPushButtonSet::New();
  pushbutton_set->SetParent(parent);
  pushbutton_set->Create(app);
  pushbutton_set->SetBorderWidth(2);
  pushbutton_set->SetReliefToGroove();
  pushbutton_set->SetWidgetsPadX(1);
  pushbutton_set->SetWidgetsPadY(1);
  pushbutton_set->SetPadX(1);
  pushbutton_set->SetPadY(1);
  pushbutton_set->ExpandWidgetsOn();

  char buffer[250];
  for (int i = 0; i <= 100; i += 25)
    {
    sprintf(buffer, "Set Progress to %d%%", i);
    vtkKWPushButton *pushbutton = pushbutton_set->AddWidget(i);
    pushbutton->SetText(buffer);
    sprintf(buffer, "SetValue %d", i);
    pushbutton->SetCommand(progress1, buffer);
    }

  // Add a special button that will iterate from 0 to 100% in Tcl

  vtkKWPushButton *pushbutton = pushbutton_set->AddWidget(1000);
  pushbutton->SetText("0% to 100%");

  sprintf(
    buffer, 
    "for {set i 0} {$i <= 100} {incr i} { %s SetValue $i ; after 20; update}",
    progress1->GetTclName());
  pushbutton->SetCommand(NULL, buffer);
  
  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    pushbutton_set->GetWidgetName());

  pushbutton_set->Delete();

  // TODO: add callbacks

  return new vtkKWProgressGaugeItem;
}
