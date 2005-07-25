#include "vtkKWChangeColorButton.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"
#include "vtkKWLabel.h"

#include "KWWidgetsTourExampleTypes.h"

class vtkKWChangeColorButtonItem : public KWWidgetsTourItem
{
public:
  virtual int GetType() { return KWWidgetsTourItem::TypeComposite; };
};

KWWidgetsTourItem* vtkKWChangeColorButtonEntryPoint(
  vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // Create a color button. The label is inside the button

  vtkKWChangeColorButton *ccb1 = vtkKWChangeColorButton::New();
  ccb1->SetParent(parent);
  ccb1->Create(app);
  ccb1->SetColor(1.0, 0.0, 0.0);
  ccb1->SetLabelPositionToLeft();
  ccb1->SetLabelText("Set Background Color");
  ccb1->SetCommand(parent, "SetBackgroundColor");
  ccb1->SetColor(vtkKWCoreWidget::SafeDownCast(parent)->GetBackgroundColor());
  ccb1->SetBalloonHelpString(
    "A color button. Note that the label is inside the button. Its position "
    "can be changed. It sets the background color of its parent.");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    ccb1->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create another ccb, but put the label and entry on top

  vtkKWChangeColorButton *ccb2 = vtkKWChangeColorButton::New();
  ccb2->SetParent(parent);
  ccb2->Create(app);
  ccb2->SetColor(0.0, 1.0, 0.0);
  ccb2->LabelOutsideButtonOn();
  ccb2->SetLabelPositionToRight();
  ccb2->SetCommand(ccb2->GetLabel(), "SetForegroundColor");
  ccb2->SetColor(ccb2->GetLabel()->GetForegroundColor());
  ccb2->SetBalloonHelpString(
    "A color button. Note that the label is now outside the button and its "
    "default position has been changed to the right. It sets the color "
    "of its own internal label.");
    
  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    ccb2->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create another color button, without a label

  vtkKWChangeColorButton *ccb3 = vtkKWChangeColorButton::New();
  ccb3->SetParent(parent);
  ccb3->Create(app);
  ccb3->SetColor(0.0, 0.0, 1.0);
  ccb3->LabelVisibilityOff();
  ccb3->SetBalloonHelpString(
    "A color button. Note that the label is now hidden.");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    ccb3->GetWidgetName());

  ccb1->Delete();
  ccb2->Delete();
  ccb3->Delete();

  return new vtkKWChangeColorButtonItem;
}
