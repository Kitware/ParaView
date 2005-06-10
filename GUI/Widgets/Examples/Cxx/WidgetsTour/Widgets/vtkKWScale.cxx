#include "vtkKWScale.h"
#include "vtkKWApplication.h"

int vtkKWScaleEntryPoint(vtkKWWidget *parent)
{
  vtkKWApplication *app = parent->GetApplication();

  vtkKWScale *scale;

  // Create a scale

  scale = vtkKWScale::New();
  scale->SetParent(parent);
  scale->Create(app, NULL);
  scale->SetRange(0.0, 100.0);
  scale->SetResolution(1.0);
  scale->SetLength(150);
  scale->DisplayEntry();
  scale->DisplayEntryAndLabelOnTopOff();
  scale->DisplayLabel("A scale:");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    scale->GetWidgetName());

  scale->Delete();

  // Create another scale, but put the label and entry on top

  scale = vtkKWScale::New();
  scale->SetParent(parent);
  scale->Create(app, NULL);
  scale->SetRange(0.0, 100.0);
  scale->SetResolution(1.0);
  scale->SetLength(350);
  scale->DisplayEntry();
  scale->DisplayEntryAndLabelOnTopOn();
  scale->DisplayLabel("A scale with label/entry on top:");
  scale->SetBalloonHelpString("This time, the label and entry are on top");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    scale->GetWidgetName());

  scale->Delete();

  // Create another scale, popup mode

  scale = vtkKWScale::New();
  scale->SetParent(parent);
  scale->PopupScaleOn();
  scale->Create(app, NULL);
  scale->SetRange(0.0, 100.0);
  scale->SetResolution(1.0);
  scale->DisplayEntry();
  scale->DisplayLabel("A popup scale:");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    scale->GetWidgetName());

  scale->Delete();

  return 1;
}
