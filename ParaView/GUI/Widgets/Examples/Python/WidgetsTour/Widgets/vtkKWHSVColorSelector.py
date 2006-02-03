from vtk import *
from kwwidgets import *


def vtkKWHSVColorSelectorEntryPoint(parent, win):

    app = parent.GetApplication()
    
    # Create a color selector
    
    ccb = vtkKWHSVColorSelector()
    ccb.SetParent(parent)
    ccb.Create()
    ccb.SetSelectionChangingCommand(parent, "SetBackgroundColor")
    ccb.InvokeCommandsWithRGBOn()
    ccb.SetBalloonHelpString(
        "This HSV Color Selector changes the background color of its parent")
    ccb.SetSelectedColor(
        vtkMath.RGBToHSV(
            vtkKWCoreWidget.SafeDownCast(parent).GetBackgroundColor()))
    
    app.Script("pack %s -side top -anchor nw -expand y -padx 2 -pady 2",
        ccb.GetWidgetName())
    
    
    
    return "TypeComposite"
