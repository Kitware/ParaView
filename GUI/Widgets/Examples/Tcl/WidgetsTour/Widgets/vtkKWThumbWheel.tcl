proc vtkKWThumbWheelEntryPoint {parent win} {

    set app [$parent GetApplication]

    # Create a thumbwheel

    vtkKWThumbWheel thumbwheel1
    thumbwheel1 SetParent $parent
    thumbwheel1 Create $app
    thumbwheel1 SetLength 150
    thumbwheel1 DisplayEntryOn
    thumbwheel1 DisplayEntryAndLabelOnTopOff
    thumbwheel1 DisplayLabelOn
    [thumbwheel1 GetLabel] SetText "A thumbwheel:"

    pack [thumbwheel1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

    # Create another thumbwheel but put the label and entry on top

    vtkKWThumbWheel thumbwheel2
    thumbwheel2 SetParent $parent
    thumbwheel2 Create $app
    thumbwheel2 SetRange -10.0 10.0
    thumbwheel2 ClampMinimumValueOn
    thumbwheel2 ClampMaximumValueOn
    thumbwheel2 SetLength 275
    thumbwheel2 SetSizeOfNotches [expr [thumbwheel2 GetSizeOfNotches] * 3]
    thumbwheel2 DisplayEntryOn
    thumbwheel2 DisplayEntryAndLabelOnTopOn
    thumbwheel2 DisplayLabelOn
    [thumbwheel2 GetLabel] SetText "A thumbwheel with label/entry on top:"
    thumbwheel2 SetBalloonHelpString "This time the label and entry are on top and we clamp the range and bigger notches"

    pack [thumbwheel2 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6
    
    # Create another thumbwheel popup mode

    vtkKWThumbWheel thumbwheel3
    thumbwheel3 SetParent $parent
    thumbwheel3 PopupModeOn
    thumbwheel3 Create $app
    thumbwheel3 SetRange 0.0 100.0
    thumbwheel3 SetResolution 1.0
    thumbwheel3 DisplayEntryOn
    thumbwheel3 DisplayLabelOn
    [thumbwheel3 GetLabel] SetText "A popup thumbwheel:"

    pack [thumbwheel3 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

    return 2
}

proc vtkKWThumbWheelFinalizePoint {} {
    thumbwheel1 Delete
    thumbwheel2 Delete
    thumbwheel3 Delete
}
