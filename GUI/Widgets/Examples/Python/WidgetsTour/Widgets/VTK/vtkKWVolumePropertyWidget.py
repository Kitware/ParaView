from kwwidgets import vtkKWFrameWithScrollbar
from vtk import vtkColorTransferFunction
from kwwidgets import vtkKWApplication
from kwwidgets import vtkKWVolumePropertyWidget
from kwwidgets import vtkKWWindow
from vtk import vtkPiecewiseFunction
from vtk import vtkVolumeProperty



def vtkKWVolumePropertyWidgetEntryPoint(parent, win):

    app = parent.GetApplication()
    
    # -----------------------------------------------------------------------
    
    # This is a faily big widget, so create a scrolled frame
    
    vpw_frame = vtkKWFrameWithScrollbar()
    vpw_frame.SetParent(parent)
    vpw_frame.Create()
    
    app.Script("pack %s -side top -fill both -expand y",
        vpw_frame.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create a volume property widget
    
    vpw = vtkKWVolumePropertyWidget()
    vpw.SetParent(vpw_frame.GetFrame())
    vpw.Create()
    
    app.Script("pack %s -side top -anchor nw -expand y -padx 2 -pady 2",
        vpw.GetWidgetName())
    
    # Create a volume property and assign it
    # We need color tfuncs, opacity, and gradient
    
    vpw_vp = vtkVolumeProperty()
    vpw_vp.SetIndependentComponents(1)
    
    vpw_cfun = vtkColorTransferFunction()
    vpw_cfun.SetColorSpaceToHSV()
    vpw_cfun.AddHSVSegment(0.0, 0.2, 1.0, 1.0, 255.0, 0.8, 1.0, 1.0)
    vpw_cfun.AddHSVSegment(80, 0.8, 1.0, 1.0, 130.0, 0.1, 1.0, 1.0)
    
    vpw_ofun = vtkPiecewiseFunction()
    vpw_ofun.AddSegment(0.0, 0.2, 255.0, 0.8)
    vpw_ofun.AddSegment(40, 0.9, 120.0, 0.1)
    
    vpw_gfun = vtkPiecewiseFunction()
    vpw_gfun.AddSegment(0.0, 0.2, 60.0, 0.4)
    
    vpw_vp.SetColor(0, vpw_cfun)
    vpw_vp.SetScalarOpacity(0, vpw_ofun)
    vpw_vp.SetGradientOpacity(0, vpw_gfun)
    
    vpw.SetVolumeProperty(vpw_vp)
    vpw.SetWindowLevel(128, 128)
    
    #vpw.MergeScalarOpacityAndColorEditors()
    
    
    
    return "TypeVTK"
