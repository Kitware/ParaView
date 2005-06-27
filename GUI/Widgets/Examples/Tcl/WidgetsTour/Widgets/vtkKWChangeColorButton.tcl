proc vtkKWChangeColorButtonEntryPoint {parent win} {

    set app [$parent GetApplication]

    # -----------------------------------------------------------------------

    # Create a color button. The label is inside the button
    
    vtkKWChangeColorButton ccb1
    ccb1 SetParent $parent
    ccb1 Create $app
    ccb1 SetColor 1.0 0.0 0.0
    ccb1 SetLabelPositionToLeft
    ccb1 SetBalloonHelpString "A color button. Note that the label is inside the button. Its position can be changed."
    
    pack [ccb1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

    # -----------------------------------------------------------------------

    # Create another color button. The label is now outside the button

    vtkKWChangeColorButton ccb2
    ccb2 SetParent $parent
    ccb2 Create $app
    ccb2 SetColor 0.0 1.0 0.0
    ccb2 LabelOutsideButtonOn
    ccb2 SetLabelPositionToRight
    ccb2 SetBalloonHelpString "A color button. Note that the label is now outside the button and its default position has been changed to the right."
    
    pack [ccb2 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

    # -----------------------------------------------------------------------

    # Create another color button, without a label

    vtkKWChangeColorButton ccb3
    ccb3 SetParent $parent
    ccb3 Create $app
    ccb3 SetColor 0.0 0.0 1.0
    ccb3 ShowLabelOff
    ccb3 SetBalloonHelpString "A color button. Note that the label is now hidden."

    pack [ccb3 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

    return 2
}

proc vtkKWChangeColorButtonFinalizePoint {} {
    ccb1 Delete
    ccb2 Delete
    ccb3 Delete
}
