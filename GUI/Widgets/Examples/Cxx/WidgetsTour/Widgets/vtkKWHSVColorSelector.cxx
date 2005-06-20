#include "vtkKWHSVColorSelector.h"
#include "vtkKWApplication.h"
#include "vtkMath.h"
#include "vtkKWWindow.h"

#include "KWWidgetsTourExampleTypes.h"

WidgetType vtkKWHSVColorSelectorEntryPoint(vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // Create a color selector

  vtkKWHSVColorSelector *ccb = vtkKWHSVColorSelector::New();
  ccb->SetParent(parent);
  ccb->Create(app);
  ccb->SetSelectionChangingCommand(parent, "SetBackgroundColor");
  ccb->InvokeCommandsWithRGBOn();

  double r, g, b, h, s, v;
  parent->GetBackgroundColor(&r, &g, &b);
  vtkMath::RGBToHSV(r, g, b, &h, &s, &v);
  ccb->SetSelectedColor(h, s, v);

  app->Script("pack %s -side top -anchor nw -expand y -padx 2 -pady 2", 
              ccb->GetWidgetName());

  ccb->Delete();

  return CompositeWidget;
}
