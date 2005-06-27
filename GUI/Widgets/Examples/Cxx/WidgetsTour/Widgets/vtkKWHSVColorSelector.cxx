#include "vtkKWHSVColorSelector.h"
#include "vtkKWApplication.h"
#include "vtkMath.h"
#include "vtkKWWindow.h"

#include "KWWidgetsTourExampleTypes.h"

class vtkKWHSVColorSelectorItem : public KWWidgetsTourItem
{
public:
  virtual int GetType() { return KWWidgetsTourItem::TypeComposite; };
};

KWWidgetsTourItem* vtkKWHSVColorSelectorEntryPoint(
  vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // Create a color selector

  vtkKWHSVColorSelector *ccb = vtkKWHSVColorSelector::New();
  ccb->SetParent(parent);
  ccb->Create(app);
  ccb->SetSelectionChangingCommand(parent, "SetBackgroundColor");
  ccb->InvokeCommandsWithRGBOn();
  ccb->SetBalloonHelpString(
    "This HSV Color Selector changes the background color of its parent");
  ccb->SetSelectedColor(vtkMath::RGBToHSV(parent->GetBackgroundColor()));

  app->Script("pack %s -side top -anchor nw -expand y -padx 2 -pady 2", 
              ccb->GetWidgetName());

  ccb->Delete();

  return new vtkKWHSVColorSelectorItem;
}
