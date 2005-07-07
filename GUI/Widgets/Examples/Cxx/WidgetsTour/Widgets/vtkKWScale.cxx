#include "vtkKWScale.h"
#include "vtkKWScaleSet.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"

#include "KWWidgetsTourExampleTypes.h"

class vtkKWScaleItem : public KWWidgetsTourItem
{
public:
  virtual int GetType() { return KWWidgetsTourItem::TypeCore; };
};

KWWidgetsTourItem* vtkKWScaleEntryPoint(vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // Create a scale

  vtkKWScale *scale1 = vtkKWScale::New();
  scale1->SetParent(parent);
  scale1->Create(app);
  scale1->SetRange(0.0, 100.0);
  scale1->SetResolution(1.0);
  scale1->SetLength(150);
  scale1->DisplayEntry();
  scale1->DisplayEntryAndLabelOnTopOff();
  scale1->DisplayLabel("A scale:");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    scale1->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create another scale, but put the label and entry on top

  vtkKWScale *scale2 = vtkKWScale::New();
  scale2->SetParent(parent);
  scale2->Create(app);
  scale2->SetRange(0.0, 100.0);
  scale2->SetResolution(1.0);
  scale2->SetLength(350);
  scale2->DisplayEntry();
  scale2->DisplayEntryAndLabelOnTopOn();
  scale2->DisplayLabel("A scale with label/entry on top:");
  scale2->SetBalloonHelpString("This time, the label and entry are on top");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    scale2->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create another scale, popup mode
  // It also sets scale2 to the same value

  vtkKWScale *scale3 = vtkKWScale::New();
  scale3->SetParent(parent);
  scale3->PopupScaleOn();
  scale3->Create(app);
  scale3->SetRange(0.0, 100.0);
  scale3->SetResolution(1.0);
  scale3->DisplayEntry();
  scale3->DisplayLabel("A popup scale:");
  scale3->SetBalloonHelpString(
    "It's a pop-up, and it sets the previous scale value too");

  char buffer[100];
  sprintf(buffer, "SetValue [%s GetValue]", scale3->GetTclName());
  scale3->SetCommand(scale2, buffer);

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    scale3->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create a set of scale
  // An easy way to create a bunch of related widgets without allocating
  // them one by one

  vtkKWScaleSet *scale_set = vtkKWScaleSet::New();
  scale_set->SetParent(parent);
  scale_set->Create(app);
  scale_set->SetBorderWidth(2);
  scale_set->SetReliefToGroove();
  scale_set->SetMaximumNumberOfWidgetsInPackingDirection(2);

  for (int i = 0; i < 4; i++)
    {
    sprintf(buffer, "Scale %d", i);
    vtkKWScale *scale = scale_set->AddWidget(i);
    scale->DisplayLabel(buffer);
    scale->SetBalloonHelpString(
      "This scale is part of a unique set (a vtkKWScaleSet), "
      "which provides an easy way to create a bunch of related widgets "
      "without allocating them one by one. The widgets can be layout as a "
      "NxM grid.");
    }

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    scale_set->GetWidgetName());

  scale1->Delete();
  scale2->Delete();
  scale3->Delete();
  scale_set->Delete();

  return new vtkKWScaleItem;
}
