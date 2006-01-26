proc vtkKWVolumePropertyWidgetEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # This is a faily big widget so create a scrolled frame

  set vpw_frame [vtkKWFrameWithScrollbar New]
  $vpw_frame SetParent $parent
  $vpw_frame Create

  pack [$vpw_frame GetWidgetName] -side top -fill both -expand y
    
  # -----------------------------------------------------------------------

  # Create a volume property widget

  set vpw [vtkKWVolumePropertyWidget New]
  $vpw SetParent [$vpw_frame GetFrame] 
  $vpw Create
 
  pack [$vpw GetWidgetName] -side top -anchor nw -expand y -padx 2 -pady 2

  # Create a volume property and assign it
  # We need color tfuncs opacity and gradient

  set vpw_vp [vtkVolumeProperty New]
  $vpw_vp SetIndependentComponents 1

  set vpw_cfun [vtkColorTransferFunction New]
  $vpw_cfun SetColorSpaceToHSV
  $vpw_cfun AddHSVSegment 0.0 0.2 1.0 1.0 255.0 0.8 1.0 1.0
  $vpw_cfun AddHSVSegment 80 0.8 1.0 1.0 130.0 0.1 1.0 1.0

  set vpw_ofun [vtkPiecewiseFunction New]
  $vpw_ofun AddSegment 0.0 0.2 255.0 0.8
  $vpw_ofun AddSegment 40 0.9 120.0 0.1
  
  set vpw_gfun [vtkPiecewiseFunction New]
  $vpw_gfun AddSegment 0.0 0.2 60.0 0.4
  
  $vpw_vp SetColor 0 $vpw_cfun
  $vpw_vp SetScalarOpacity 0 $vpw_ofun
  $vpw_vp SetGradientOpacity 0 $vpw_gfun

  $vpw SetVolumeProperty $vpw_vp
  $vpw SetWindowLevel 128 128

  return "TypeVTK"
}
