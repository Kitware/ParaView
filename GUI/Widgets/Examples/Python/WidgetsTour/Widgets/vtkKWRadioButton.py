from kwwidgets import vtkKWRadioButton
from kwwidgets import vtkKWRadioButton
from kwwidgets import vtkKWRadioButtonSet
from kwwidgets import vtkKWApplication
from kwwidgets import vtkKWWindow
from kwwidgets import vtkKWIcon



def vtkKWRadioButtonEntryPoint(parent, win):

    app = parent.GetApplication()
    
    # -----------------------------------------------------------------------
    
    # Create two radiobuttons.
    # They share the same variable name and each one has a different internal
    # value.
    
    radiob1 = vtkKWRadioButton()
    radiob1.SetParent(parent)
    radiob1.Create()
    radiob1.SetText("A radiobutton")
    radiob1.SetValueAsInt(123)
    
    radiob1b = vtkKWRadioButton()
    radiob1b.SetParent(parent)
    radiob1b.Create()
    radiob1b.SetText("Another radiobutton")
    radiob1b.SetValueAsInt(456)
    
    radiob1.SetSelectedState(1)
    radiob1b.SetVariableName(radiob1.GetVariableName())
    
    app.Script("pack %s -side top -anchor nw -expand n -padx 2 -pady 2",
        radiob1.GetWidgetName())
    app.Script("pack %s -side top -anchor nw -expand n -padx 2 -pady 2",
        radiob1b.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create two radiobuttons. Use icons
    # They share the same variable name and each one has a different internal
    # value.
    
    radiob2 = vtkKWRadioButton()
    radiob2.SetParent(parent)
    radiob2.Create()
    radiob2.SetImageToPredefinedIcon(vtkKWIcon.IconPlus)
    radiob2.IndicatorVisibilityOff()
    radiob2.SetValue("foo")
    
    radiob2b = vtkKWRadioButton()
    radiob2b.SetParent(parent)
    radiob2b.Create()
    radiob2b.SetImageToPredefinedIcon(vtkKWIcon.IconMinus)
    radiob2b.IndicatorVisibilityOff()
    radiob2b.SetValue("bar")
    
    radiob2.SetSelectedState(1)
    radiob2b.SetVariableName(radiob2.GetVariableName())
    
    app.Script("pack %s -side top -anchor nw -expand n -padx 2 -pady 2",
        radiob2.GetWidgetName())
    app.Script("pack %s -side top -anchor nw -expand n -padx 2 -pady 2",
        radiob2b.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create two radiobuttons. Use both labels and icons
    # They share the same variable name and each one has a different internal
    # value.
    
    radiob3 = vtkKWRadioButton()
    radiob3.SetParent(parent)
    radiob3.Create()
    radiob3.SetText("Linear")
    radiob3.SetImageToPredefinedIcon(vtkKWIcon.IconGridLinear)
    radiob3.SetCompoundModeToLeft()
    radiob3.IndicatorVisibilityOff()
    radiob3.SetValue("foo")
    
    radiob3b = vtkKWRadioButton()
    radiob3b.SetParent(parent)
    radiob3b.Create()
    radiob3b.SetText("Log")
    radiob3b.SetImageToPredefinedIcon(vtkKWIcon.IconGridLog)
    radiob3b.SetCompoundModeToLeft()
    radiob3b.IndicatorVisibilityOff()
    radiob3b.SetValue("bar")
    
    radiob3.SetSelectedState(1)
    radiob3b.SetVariableName(radiob3.GetVariableName())
    
    app.Script("pack %s -side top -anchor nw -expand n -padx 2 -pady 2",
        radiob3.GetWidgetName())
    app.Script("pack %s -side top -anchor nw -expand n -padx 2 -pady 2",
        radiob3b.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create a set of radiobutton
    # An easy way to create a bunch of related widgets without allocating
    # them one by one
    
    radiob_set = vtkKWRadioButtonSet()
    radiob_set.SetParent(parent)
    radiob_set.Create()
    radiob_set.SetBorderWidth(2)
    radiob_set.SetReliefToGroove()
    
    for id in range(0,4):
        buffer = "Radiobutton %d" % (id)
        radiob = radiob_set.AddWidget(id)
        radiob.SetText(buffer)
        radiob.SetBalloonHelpString(
            "This radiobutton is part of a unique set (a vtkKWRadioButtonSet), "
            "which provides an easy way to create a bunch of related widgets "
            "without allocating them one by one. The widgets can be layout as a "
            "NxM grid. This classes automatically set the same variable name "
            "among all radiobuttons, as well as a unique value.")
        
    
    radiob_set.GetWidget(0).SetSelectedState(1)
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 6",
        radiob_set.GetWidgetName())
    
    
    # -----------------------------------------------------------------------
    
    # TODO: use vtkKWRadioButtonSetWithLabel and callbacks
    
    
    return "TypeCore"
