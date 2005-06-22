proc vtkKWHSVColorSelectorEntryPoint {parent win} {

    set app [$parent GetApplication]

    # Create a color selector

    vtkKWHSVColorSelector ccb
    ccb SetParent $parent
    ccb Create $app
    ccb SetSelectionChangingCommand $parent "SetBackgroundColor"
    ccb InvokeCommandsWithRGBOn
    ccb SetBalloonHelpString "This HSV Color Selector changes the background color of its parent"

    vtkMath math
    eval ccb SetSelectedColor [eval math RGBToHSV [$parent GetBackgroundColor]]

    pack [ccb GetWidgetName] -side top -anchor nw -expand y -padx 2 -pady 2

    math Delete

    return 2
}

proc vtkKWHSVColorSelectorFinalizePoint {} {
    ccb Delete
}