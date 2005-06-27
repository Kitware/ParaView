#include "vtkKWApplication.h"
#include "vtkKWLabel.h"
#include "vtkKWThumbWheel.h"
#include "vtkKWWindow.h"

#include "KWWidgetsTourExampleTypes.h"

class vtkKWThumbWheelItem : public KWWidgetsTourItem
{
public:
  virtual int GetType() { return KWWidgetsTourItem::TypeComposite; };
};

KWWidgetsTourItem* vtkKWThumbWheelEntryPoint(
  vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // Create a thumbwheel

  vtkKWThumbWheel *thumbwheel1 = vtkKWThumbWheel::New();
  thumbwheel1->SetParent(parent);
  thumbwheel1->Create(app);
  thumbwheel1->SetLength(150);
  thumbwheel1->DisplayEntryOn();
  thumbwheel1->DisplayEntryAndLabelOnTopOff();
  thumbwheel1->DisplayLabelOn();
  thumbwheel1->GetLabel()->SetText("A thumbwheel:");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    thumbwheel1->GetWidgetName());

  thumbwheel1->Delete();

  // -----------------------------------------------------------------------

  // Create another thumbwheel, but put the label and entry on top

  vtkKWThumbWheel *thumbwheel2 = vtkKWThumbWheel::New();
  thumbwheel2->SetParent(parent);
  thumbwheel2->Create(app);
  thumbwheel2->SetRange(-10.0, 10.0);
  thumbwheel2->ClampMinimumValueOn();
  thumbwheel2->ClampMaximumValueOn();
  thumbwheel2->SetLength(275);
  thumbwheel2->SetSizeOfNotches(thumbwheel2->GetSizeOfNotches() * 3);
  thumbwheel2->DisplayEntryOn();
  thumbwheel2->DisplayEntryAndLabelOnTopOn();
  thumbwheel2->DisplayLabelOn();
  thumbwheel2->GetLabel()->SetText("A thumbwheel with label/entry on top:");
  thumbwheel2->SetBalloonHelpString(
    "This time, the label and entry are on top, and we clamp the range, "
    "and bigger notches");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    thumbwheel2->GetWidgetName());

  thumbwheel2->Delete();

  // -----------------------------------------------------------------------

  // Create another thumbwheel, popup mode

  vtkKWThumbWheel *thumbwheel3 = vtkKWThumbWheel::New();
  thumbwheel3->SetParent(parent);
  thumbwheel3->PopupModeOn();
  thumbwheel3->Create(app);
  thumbwheel3->SetRange(0.0, 100.0);
  thumbwheel3->SetResolution(1.0);
  thumbwheel3->DisplayEntryOn();
  thumbwheel3->DisplayLabelOn();
  thumbwheel3->GetLabel()->SetText("A popup thumbwheel:");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    thumbwheel3->GetWidgetName());

  thumbwheel3->Delete();

  return new vtkKWThumbWheelItem;
}
