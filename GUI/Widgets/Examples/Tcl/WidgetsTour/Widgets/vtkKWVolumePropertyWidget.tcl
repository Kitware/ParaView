package require vtkrendering

proc vtkKWVolumePropertyWidgetEntryPoint {parent} {

    global objects
    set app [$parent GetApplication]

    # Create a volume property widget

    vtkKWVolumePropertyWidget vpw
    vpw SetParent $parent
    vpw Create app ""
    
    pack [vpw GetWidgetName] -side top -anchor nw -expand y -padx 2 -pady 2

    lappend objects vpw

    # Create a volume property and assign it
    # We need color tfuncs opacity and gradient

    vtkVolumeProperty vp
    vp SetIndependentComponents 1

    vtkColorTransferFunction cfun
    cfun AddHSVSegment 0.0 0.2 1.0 1.0 255.0 0.8 1.0 1.0
    cfun AddHSVSegment 80 0.8 1.0 1.0 130.0 0.1 1.0 1.0

    vtkPiecewiseFunction ofun
    ofun AddSegment 0.0 0.2 255.0 0.8
    ofun AddSegment 40 0.9 120.0 0.1
    
    vtkPiecewiseFunction gfun
    gfun AddSegment 0.0 0.2 60.0 0.4
    
    vp SetColor 0 cfun
    vp SetScalarOpacity 0 ofun
    vp SetGradientOpacity 0 gfun

    lappend objects cfun
    lappend objects ofun
    lappend objects gfun

# TODO: this can't be done because of PV
#    vpw SetVolumeProperty vp
    vpw SetWindowLevel 128 128

    lappend objects vp

    return 1
}
