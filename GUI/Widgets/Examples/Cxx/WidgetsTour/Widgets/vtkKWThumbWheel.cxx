#include "vtkKWThumbWheel.h"
#include "vtkKWApplication.h"
#include "vtkKWLabel.h"

int vtkKWThumbWheelEntryPoint(vtkKWWidget *parent)
{
  vtkKWApplication *app = parent->GetApplication();

  vtkKWThumbWheel *thumbwheel;

  // Create a thumbwheel

  thumbwheel = vtkKWThumbWheel::New();
  thumbwheel->SetParent(parent);
  thumbwheel->Create(app, NULL);
  thumbwheel->SetLength(150);
  thumbwheel->DisplayEntryOn();
  thumbwheel->DisplayEntryAndLabelOnTopOff();
  thumbwheel->DisplayLabelOn();
  thumbwheel->GetLabel()->SetText("A thumbwheel:");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    thumbwheel->GetWidgetName());

  thumbwheel->Delete();

  // Create another thumbwheel, but put the label and entry on top

  thumbwheel = vtkKWThumbWheel::New();
  thumbwheel->SetParent(parent);
  thumbwheel->Create(app, NULL);
  thumbwheel->SetRange(-10.0, 10.0);
  thumbwheel->ClampMinimumValueOn();
  thumbwheel->ClampMaximumValueOn();
  thumbwheel->SetLength(275);
  thumbwheel->SetSizeOfNotches(thumbwheel->GetSizeOfNotches() * 3);
  thumbwheel->DisplayEntryOn();
  thumbwheel->DisplayEntryAndLabelOnTopOn();
  thumbwheel->DisplayLabelOn();
  thumbwheel->GetLabel()->SetText("A thumbwheel with label/entry on top:");
  thumbwheel->SetBalloonHelpString(
    "This time, the label and entry are on top, and we clamp the range, "
    "and bigger notches");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    thumbwheel->GetWidgetName());

  thumbwheel->Delete();

  // Create another thumbwheel, popup mode

  thumbwheel = vtkKWThumbWheel::New();
  thumbwheel->SetParent(parent);
  thumbwheel->PopupModeOn();
  thumbwheel->Create(app, NULL);
  thumbwheel->SetRange(0.0, 100.0);
  thumbwheel->SetResolution(1.0);
  thumbwheel->DisplayEntryOn();
  thumbwheel->DisplayLabelOn();
  thumbwheel->GetLabel()->SetText("A popup thumbwheel:");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    thumbwheel->GetWidgetName());

  thumbwheel->Delete();

  return 1;
}
