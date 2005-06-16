#include "vtkKWFrameWithScrollbar.h"
#include "vtkColorTransferFunction.h"
#include "vtkKWApplication.h"
#include "vtkKWVolumePropertyWidget.h"
#include "vtkKWWindow.h"
#include "vtkPiecewiseFunction.h"
#include "vtkVolumeProperty.h"

int vtkKWVolumePropertyWidgetEntryPoint(vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // This is a faily big widget, so create a scrolled frame

  vtkKWFrameWithScrollbar *framews = vtkKWFrameWithScrollbar::New();
  framews->SetParent(parent);
  framews->Create(app);

  app->Script("pack %s -side top -fill both -expand y", 
              framews->GetWidgetName());
    
  framews->Delete();

  // Create a volume property widget

  vtkKWVolumePropertyWidget *vpw = vtkKWVolumePropertyWidget::New();
  vpw->SetParent(framews->GetFrame());
  vpw->Create(app);
 
  app->Script("pack %s -side top -anchor nw -expand y -padx 2 -pady 2", 
              vpw->GetWidgetName());

  vpw->Delete();

  // Create a volume property and assign it
  // We need color tfuncs, opacity, and gradient

  vtkVolumeProperty *vp = vtkVolumeProperty::New();
  vp->SetIndependentComponents(1);

  vtkColorTransferFunction *cfun = vtkColorTransferFunction::New();
  cfun->AddHSVSegment(0.0, 0.2, 1.0, 1.0, 255.0, 0.8, 1.0, 1.0);
  cfun->AddHSVSegment(80, 0.8, 1.0, 1.0, 130.0, 0.1, 1.0, 1.0);

  vtkPiecewiseFunction *ofun = vtkPiecewiseFunction::New();
  ofun->AddSegment(0.0, 0.2, 255.0, 0.8);
  ofun->AddSegment(40, 0.9, 120.0, 0.1);
  
  vtkPiecewiseFunction *gfun = vtkPiecewiseFunction::New();
  gfun->AddSegment(0.0, 0.2, 60.0, 0.4);
  
  vp->SetColor(0, cfun);
  vp->SetScalarOpacity(0, ofun);
  vp->SetGradientOpacity(0, gfun);

  cfun->Delete();
  ofun->Delete();
  gfun->Delete();

  vpw->SetVolumeProperty(vp);
  vpw->SetWindowLevel(128, 128);

  vp->Delete();

  return 1;
}
