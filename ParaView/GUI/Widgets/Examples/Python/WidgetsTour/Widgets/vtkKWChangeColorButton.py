from kwwidgets import vtkKWChangeColorButton
from kwwidgets import vtkKWApplication
from kwwidgets import vtkKWWindow
from kwwidgets import vtkKWLabel



def vtkKWChangeColorButtonEntryPoint(parent, win):

    app = parent.GetApplication()
    
    # -----------------------------------------------------------------------
    
    # Create a color button. The label is inside the button
    
    ccb1 = vtkKWChangeColorButton()
    ccb1.SetParent(parent)
    ccb1.Create()
    ccb1.SetColor(1.0, 0.0, 0.0)
    ccb1.SetLabelPositionToLeft()
    ccb1.SetLabelText("Set Background Color")
    ccb1.SetCommand(parent, "SetBackgroundColor")
    ccb1.SetColor(parent.GetBackgroundColor())
    ccb1.SetBalloonHelpString(
        "A color button. Note that the label is inside the button. Its position "
        "can be changed. It sets the background color of its parent.")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 2",
        ccb1.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create another ccb, but put the label and entry on top
    
    ccb2 = vtkKWChangeColorButton()
    ccb2.SetParent(parent)
    ccb2.Create()
    ccb2.SetColor(0.0, 1.0, 0.0)
    ccb2.LabelOutsideButtonOn()
    ccb2.SetLabelPositionToRight()
    ccb2.SetCommand(ccb2.GetLabel(), "SetForegroundColor")
    ccb2.SetColor(ccb2.GetLabel().GetForegroundColor())
    ccb2.SetBalloonHelpString(
        "A color button. Note that the label is now outside the button and its "
        "default position has been changed to the right. It sets the color "
        "of its own internal label.")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 6",
        ccb2.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create another color button, without a label
    
    ccb3 = vtkKWChangeColorButton()
    ccb3.SetParent(parent)
    ccb3.Create()
    ccb3.SetColor(0.0, 0.0, 1.0)
    ccb3.LabelVisibilityOff()
    ccb3.SetBalloonHelpString(
        "A color button. Note that the label is now hidden.")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 6",
        ccb3.GetWidgetName())
    
    
    
    return "TypeComposite"
