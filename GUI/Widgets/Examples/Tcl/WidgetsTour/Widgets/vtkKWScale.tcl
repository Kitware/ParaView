proc vtkKWScaleEntryPoint {parent win} {

    global objects
    set app [$parent GetApplication]

    # Create a scale
    
    vtkKWScale scale1
    scale1 SetParent $parent
    scale1 Create $app ""
    scale1 SetRange 0.0 100.0
    scale1 SetResolution 1.0
    scale1 SetLength 150
    scale1 DisplayEntry
    scale1 DisplayEntryAndLabelOnTopOff
    scale1 DisplayLabel "A scale:"
    
    pack [scale1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

    # Create another scale, but put the label and entry on top

    vtkKWScale scale2
    scale2 SetParent $parent
    scale2 Create app
    scale2 SetRange 0.0 100.0
    scale2 SetResolution 1.0
    scale2 SetLength 350
    scale2 DisplayEntry
    scale2 DisplayEntryAndLabelOnTopOn
    scale2 DisplayLabel "A scale with label/entry on top:"
    scale2 SetBalloonHelpString "This time, the label and entry are on top"

    pack [scale2 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

    # Create another scale, popup mode

    vtkKWScale scale3
    scale3 SetParent $parent
    scale3 PopupScaleOn
    scale3 Create app
    scale3 SetRange 0.0 100.0
    scale3 SetResolution 1.0
    scale3 DisplayEntry
    scale3 DisplayLabel "A popup scale:"

    pack [scale3 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

    return 1
}

proc vtkKWScaleFinalizePoint {} {
    scale1 Delete
    scale2 Delete
    scale3 Delete
}
