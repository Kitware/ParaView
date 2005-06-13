proc vtkKWHSVColorSelectorEntryPoint {parent win} {

    global objects
    set app [$parent GetApplication]

    # Create a color selector

    vtkKWHSVColorSelector ccb
    ccb SetParent $parent
    ccb Create app ""
    ccb SetSelectionChangingCommand $parent "SetBackgroundColor"
    ccb InvokeCommandsWithRGBOn

    vtkMath math
    eval ccb SetSelectedColor [eval math RGBToHSV [$parent GetBackgroundColor]]

    pack [ccb GetWidgetName] -side top -anchor nw -expand y -padx 2 -pady 2

    math Delete

    return 1
}

proc vtkKWHSVColorSelectorFinalizePoint {} {
    ccb Delete
}