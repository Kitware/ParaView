#include "vtkKWProgressGauge.h"
#include "vtkKWPushButtonSet.h"
#include "vtkKWPushButton.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"

#include <vtksys/stl/string>
#include "vtkKWWidgetsTourExample.h"

class vtkKWProgressGaugeItem : public KWWidgetsTourItem
{
public:
  virtual int GetType();
  virtual void Create(vtkKWWidget *parent, vtkKWWindow *);
};

void vtkKWProgressGaugeItem::Create(vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // Create a progress gauge

  vtkKWProgressGauge *progress1 = vtkKWProgressGauge::New();
  progress1->SetParent(parent);
  progress1->Create();
  progress1->SetWidth(150);
  progress1->SetBorderWidth(2);
  progress1->SetReliefToGroove();
  progress1->SetPadX(2);
  progress1->SetPadY(2);

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    progress1->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create a set of pushbutton that will modify the progress gauge

  vtkKWPushButtonSet *progress1_pbs = vtkKWPushButtonSet::New();
  progress1_pbs->SetParent(parent);
  progress1_pbs->Create();
  progress1_pbs->SetBorderWidth(2);
  progress1_pbs->SetReliefToGroove();
  progress1_pbs->SetWidgetsPadX(1);
  progress1_pbs->SetWidgetsPadY(1);
  progress1_pbs->SetPadX(1);
  progress1_pbs->SetPadY(1);
  progress1_pbs->ExpandWidgetsOn();

  char buffer[250];
  for (int id = 0; id <= 100; id += 25)
    {
    sprintf(buffer, "Set Progress to %d%%", id);
    vtkKWPushButton *pushbutton = progress1_pbs->AddWidget(id);
    pushbutton->SetText(buffer);
    sprintf(buffer, "SetValue %d", id);
    pushbutton->SetCommand(progress1, buffer);
    }

  // Add a special button that will iterate from 0 to 100% in Tcl

  vtkKWPushButton *pushbutton = progress1_pbs->AddWidget(1000);
  pushbutton->SetText("0% to 100%");

  sprintf(
    buffer, 
    "for {set i 0} {$i <= 100} {incr i} { %s SetValue $i ; after 20; update}",
    progress1->GetTclName());
  pushbutton->SetCommand(NULL, buffer);
  
  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    progress1_pbs->GetWidgetName());

  progress1->Delete();
  progress1_pbs->Delete();

  // TODO: add callbacks
}

int vtkKWProgressGaugeItem::GetType()
{
  return KWWidgetsTourItem::TypeComposite;
}

KWWidgetsTourItem* vtkKWProgressGaugeEntryPoint()
{
  return new vtkKWProgressGaugeItem();
}
