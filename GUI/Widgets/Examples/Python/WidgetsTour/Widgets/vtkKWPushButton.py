from kwwidgets import vtkKWPushButton
from kwwidgets import vtkKWPushButtonWithLabel
from kwwidgets import vtkKWPushButtonSet
from kwwidgets import vtkKWApplication
from kwwidgets import vtkKWWindow
from kwwidgets import vtkKWIcon
from vtk import vtkMath



def vtkKWPushButtonEntryPoint(parent, win):

    app = parent.GetApplication()
    
    # -----------------------------------------------------------------------
    
    # Create a push button
    
    pushbutton1 = vtkKWPushButton()
    pushbutton1.SetParent(parent)
    pushbutton1.Create()
    pushbutton1.SetText("A push button")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 2",
        pushbutton1.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create another push button, use an icon
    
    pushbutton2 = vtkKWPushButton()
    pushbutton2.SetParent(parent)
    pushbutton2.Create()
    pushbutton2.SetImageToPredefinedIcon(vtkKWIcon.IconConnection)
    pushbutton2.SetBalloonHelpString(
        "Another pushbutton, using one of the predefined icons")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 6",
        pushbutton2.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create another push button, use both text and icon
    
    pushbutton2b = vtkKWPushButton()
    pushbutton2b.SetParent(parent)
    pushbutton2b.Create()
    pushbutton2b.SetText("A push button with an icon")
    pushbutton2b.SetImageToPredefinedIcon(vtkKWIcon.IconWarningMini)
    pushbutton2b.SetCompoundModeToLeft()
    pushbutton2b.SetBalloonHelpString(
        "Another pushbutton, using both a text and one of the predefined icons")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 6",
        pushbutton2b.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create another push button, with a label this time
    
    pushbutton3 = vtkKWPushButtonWithLabel()
    pushbutton3.SetParent(parent)
    pushbutton3.Create()
    pushbutton3.SetLabelText("Press this...")
    pushbutton3.GetWidget().SetText("button")
    pushbutton3.SetBalloonHelpString(
        "This is a vtkKWPushButtonWithLabel, i.e. a pushbutton associated to a "
        "label that can be positioned around the pushbutton.")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 6",
        pushbutton3.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create a set of pushbutton
    # An easy way to create a bunch of related widgets without allocating
    # them one by one
    
    pushbutton_set = vtkKWPushButtonSet()
    pushbutton_set.SetParent(parent)
    pushbutton_set.Create()
    pushbutton_set.SetBorderWidth(2)
    pushbutton_set.SetReliefToGroove()
    pushbutton_set.SetWidgetsPadX(1)
    pushbutton_set.SetWidgetsPadY(1)
    pushbutton_set.SetPadX(1)
    pushbutton_set.SetPadY(1)
    pushbutton_set.ExpandWidgetsOn()
    pushbutton_set.SetMaximumNumberOfWidgetsInPackingDirection(3)
    
    for id in range(0,9):
        buffer = "Push button %d" % (id)
        pushbutton = pushbutton_set.AddWidget(id)
        pushbutton.SetText(buffer)
        pushbutton.SetBackgroundColor(
            vtkMath.HSVToRGB(float(id) / 8.0, 0.3, 0.75))
        pushbutton.SetBalloonHelpString(
            "This pushbutton is part of a unique set (a vtkKWPushButtonSet), "
            "which provides an easy way to create a bunch of related widgets "
            "without allocating them one by one. The widgets can be layout as a "
            "NxM grid. Each button is assigned a different color.")
        
    
    pushbutton_set.GetWidget(0).SetText("I'm the first button")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 6",
        pushbutton_set.GetWidgetName())
    
    
    # TODO: add callbacks
    
    
    return "TypeCore"
