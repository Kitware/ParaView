#include "vtkKWFrameWithScrollbar.h"
#include "vtkColorTransferFunction.h"
#include "vtkKWApplication.h"
#include "vtkKWVolumePropertyWidget.h"
#include "vtkKWWindow.h"
#include "vtkPiecewiseFunction.h"
#include "vtkVolumeProperty.h"

#include "KWWidgetsTourExampleTypes.h"

class vtkKWVolumePropertyWidgetItem : public KWWidgetsTourItem
{
public:
  virtual int GetType() { return KWWidgetsTourItem::TypeVTK; };
};

KWWidgetsTourItem* vtkKWVolumePropertyWidgetEntryPoint(
  vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // This is a faily big widget, so create a scrolled frame

  vtkKWFrameWithScrollbar *vpw_frame = vtkKWFrameWithScrollbar::New();
  vpw_frame->SetParent(parent);
  vpw_frame->Create(app);

  app->Script("pack %s -side top -fill both -expand y", 
              vpw_frame->GetWidgetName());
    
  // -----------------------------------------------------------------------

  // Create a volume property widget

  vtkKWVolumePropertyWidget *vpw = vtkKWVolumePropertyWidget::New();
  vpw->SetParent(vpw_frame->GetFrame());
  vpw->Create(app);

  app->Script("pack %s -side top -anchor nw -expand y -padx 2 -pady 2", 
              vpw->GetWidgetName());

  // Create a volume property and assign it
  // We need color tfuncs, opacity, and gradient

  vtkVolumeProperty *vpw_vp = vtkVolumeProperty::New();
  vpw_vp->SetIndependentComponents(1);

  vtkColorTransferFunction *vpw_cfun = vtkColorTransferFunction::New();
  vpw_cfun->SetColorSpaceToHSV();
  vpw_cfun->AddHSVSegment(0.0, 0.2, 1.0, 1.0, 255.0, 0.8, 1.0, 1.0);
  vpw_cfun->AddHSVSegment(80, 0.8, 1.0, 1.0, 130.0, 0.1, 1.0, 1.0);

  vtkPiecewiseFunction *vpw_ofun = vtkPiecewiseFunction::New();
  vpw_ofun->AddSegment(0.0, 0.2, 255.0, 0.8);
  vpw_ofun->AddSegment(40, 0.9, 120.0, 0.1);
  
  vtkPiecewiseFunction *vpw_gfun = vtkPiecewiseFunction::New();
  vpw_gfun->AddSegment(0.0, 0.2, 60.0, 0.4);
  
  vpw_vp->SetColor(0, vpw_cfun);
  vpw_vp->SetScalarOpacity(0, vpw_ofun);
  vpw_vp->SetGradientOpacity(0, vpw_gfun);

  vpw->SetVolumeProperty(vpw_vp);
  vpw->SetWindowLevel(128, 128);

  vpw_frame->Delete();
  vpw->Delete();
  vpw_cfun->Delete();
  vpw_ofun->Delete();
  vpw_gfun->Delete();
  vpw_vp->Delete();

  return new vtkKWVolumePropertyWidgetItem;
}
