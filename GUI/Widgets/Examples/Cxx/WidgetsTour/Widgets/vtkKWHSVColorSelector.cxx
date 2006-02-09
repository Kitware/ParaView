#include "vtkKWHSVColorSelector.h"
#include "vtkKWApplication.h"
#include "vtkMath.h"
#include "vtkKWWindow.h"

#include "vtkKWWidgetsTourExample.h"

class vtkKWHSVColorSelectorItem : public KWWidgetsTourItem
{
public:
  virtual int GetType();
  virtual void Create(vtkKWWidget *parent, vtkKWWindow *win);
};

void vtkKWHSVColorSelectorItem::Create(vtkKWWidget *parent, vtkKWWindow *win)
{
  vtkKWApplication *app = parent->GetApplication();

  // Create a color selector

  vtkKWHSVColorSelector *ccb = vtkKWHSVColorSelector::New();
  ccb->SetParent(parent);
  ccb->Create();
  ccb->SetSelectionChangingCommand(parent, "SetBackgroundColor");
  ccb->InvokeCommandsWithRGBOn();
  ccb->SetBalloonHelpString(
    "This HSV Color Selector changes the background color of its parent");
  ccb->SetSelectedColor(
    vtkMath::RGBToHSV(
      vtkKWCoreWidget::SafeDownCast(parent)->GetBackgroundColor()));

  app->Script("pack %s -side top -anchor nw -expand y -padx 2 -pady 2", 
              ccb->GetWidgetName());

  ccb->Delete();
}

int vtkKWHSVColorSelectorItem::GetType()
{
  return KWWidgetsTourItem::TypeComposite;
}

KWWidgetsTourItem* vtkKWHSVColorSelectorEntryPoint()
{
  return new vtkKWHSVColorSelectorItem();
}
