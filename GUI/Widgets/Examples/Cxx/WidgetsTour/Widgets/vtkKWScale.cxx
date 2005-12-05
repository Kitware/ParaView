#include "vtkKWScale.h"
#include "vtkKWScaleWithEntry.h"
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
  scale1->Create();
  scale1->SetRange(0.0, 100.0);
  scale1->SetResolution(1.0);
  scale1->SetLength(150);
  scale1->SetLabelText("A simple scale:");

  app->Script(
    "pack %s -side top -anchor nw -expand n -fill none -padx 2 -pady 2", 
    scale1->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create another scale, but put the label and entry on top

  vtkKWScaleWithEntry *scale2 = vtkKWScaleWithEntry::New();
  scale2->SetParent(parent);
  scale2->Create();
  scale2->SetRange(0.0, 100.0);
  scale2->SetResolution(1.0);
  //  scale2->GetScale()->SetLength(350);
  scale2->RangeVisibilityOn();
  scale2->SetLabelText("A more complex scale:");
  scale2->SetBalloonHelpString(
    "The vtkKWScaleWithEntry class allows a label and entry to be displayed "
    "at different location around the scale (here, on the side). The range "
    "of the scale can be displayed on the side too.");

  app->Script(
    "pack %s -side top -anchor nw -expand n -fill none -padx 2 -pady 6", 
    scale2->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create another scale, different layout, with range

  vtkKWScaleWithEntry *scale2b = vtkKWScaleWithEntry::New();
  scale2b->SetParent(parent);
  scale2b->Create();
  scale2b->SetRange(0.0, 100.0);
  scale2b->SetResolution(1.0);
  scale2b->GetScale()->SetLength(350);
  scale2b->SetLabelText("Another scale:");
  scale2b->SetLabelPositionToTop();
  scale2b->SetEntryPositionToTop();
  scale2b->SetBalloonHelpString(
    "This time the label and entry are displayed on top, and the range is "
    "not visible.");

  app->Script(
    "pack %s -side top -anchor nw -expand n -fill none -padx 2 -pady 6", 
    scale2b->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create another scale, popup mode
  // It also sets scale2b to the same value

  vtkKWScaleWithEntry *scale3 = vtkKWScaleWithEntry::New();
  scale3->SetParent(parent);
  scale3->PopupModeOn();
  scale3->Create();
  scale3->SetRange(0.0, 100.0);
  scale3->SetResolution(1.0);
  scale3->SetLabelText("A popup scale:");
  scale3->SetBalloonHelpString(
    "It's a pop-up, and it sets the previous scale value too");

  scale3->SetCommand(scale2b, "SetValue");

  app->Script(
    "pack %s -side top -anchor nw -expand n -fill none -padx 2 -pady 6", 
    scale3->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create a set of scale
  // An easy way to create a bunch of related widgets without allocating
  // them one by one

  vtkKWScaleSet *scale_set = vtkKWScaleSet::New();
  scale_set->SetParent(parent);
  scale_set->Create();
  scale_set->SetBorderWidth(2);
  scale_set->SetReliefToGroove();
  scale_set->SetMaximumNumberOfWidgetsInPackingDirection(2);

  char buffer[100];
  for (int id = 0; id < 4; id++)
    {
    sprintf(buffer, "Scale %d", id);
    vtkKWScale *scale = scale_set->AddWidget(id);
    scale->SetLabelText(buffer);
    scale->SetOrientationToVertical();
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
  scale2b->Delete();
  scale3->Delete();
  scale_set->Delete();

  return new vtkKWScaleItem;
}
