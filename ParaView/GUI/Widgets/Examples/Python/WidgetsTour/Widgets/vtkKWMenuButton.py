from kwwidgets import *


def vtkKWMenuButtonEntryPoint(parent, win):

    app = parent.GetApplication()
    
    days = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday"]
    
    # -----------------------------------------------------------------------
    
    # Create a menu button
    # Add some entries
    
    menubutton1 = vtkKWMenuButton()
    menubutton1.SetParent(parent)
    menubutton1.Create()
    menubutton1.SetBalloonHelpString("A simple menu button")
    
    for i in range(0,len(days)):
        menubutton1.AddRadioButton(days[i])
        
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 2",
        menubutton1.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create a menu button with spin buttons
    
    menubutton1b = vtkKWMenuButtonWithSpinButtons()
    menubutton1b.SetParent(parent)
    menubutton1b.Create()
    menubutton1b.GetWidget().SetWidth(20)
    menubutton1b.SetBalloonHelpString(
        "This is a vtkKWMenuButtonWithSpinButtons, i.e. a menu button associated "
        "to a set of spin buttons (vtkKWSpinButtons) that can be used to "
        "increment and decrement the value")
    
    for i in range(0,len(days)):
        menubutton1b.GetWidget().AddRadioButton(days[i])
        
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 6",
        menubutton1b.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create another menu button, this time with a label
    
    menubutton2 = vtkKWMenuButtonWithLabel()
    menubutton2.SetParent(parent)
    menubutton2.Create()
    menubutton2.SetBorderWidth(2)
    menubutton2.SetReliefToGroove()
    menubutton2.SetLabelText("Days:")
    menubutton2.SetPadX(2)
    menubutton2.SetPadY(2)
    menubutton2.GetWidget().IndicatorVisibilityOff()
    menubutton2.GetWidget().SetWidth(20)
    menubutton2.SetBalloonHelpString(
        "This is a vtkKWMenuButtonWithLabel, i.e. a menu button associated to a "
        "label that can be positioned around the menu button. The indicator is "
        "hidden, and the width is set explicitly")
    
    for i in range(0,len(days)):
        menubutton2.GetWidget().AddRadioButton(days[i])
        
    
    menubutton2.GetWidget().SetValue(days[0])
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 6",
        menubutton2.GetWidgetName())
    
    
    # TODO: use callbacks
    
    
    return "TypeCore"
