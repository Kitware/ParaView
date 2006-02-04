from kwwidgets import vtkKWSpinBox
from kwwidgets import vtkKWSpinBoxWithLabel
from kwwidgets import vtkKWApplication
from kwwidgets import vtkKWWindow



def vtkKWSpinBoxEntryPoint(parent, win):

    app = parent.GetApplication()
    
    # -----------------------------------------------------------------------
    
    # Create a spinbox
    
    spinbox1 = vtkKWSpinBox()
    spinbox1.SetParent(parent)
    spinbox1.Create()
    spinbox1.SetRange(0, 10)
    spinbox1.SetIncrement(1)
    spinbox1.SetBalloonHelpString("A simple spinbox")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 2",
        spinbox1.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create another spinbox, but larger, and read-only
    
    spinbox2 = vtkKWSpinBox()
    spinbox2.SetParent(parent)
    spinbox2.Create()
    spinbox2.SetRange(10.0, 15.0)
    spinbox2.SetIncrement(0.5)
    spinbox2.SetValue(12)
    spinbox2.SetValueFormat("%.1f")
    spinbox2.SetWidth(5)
    spinbox2.WrapOn()
    spinbox2.SetBalloonHelpString("Another spinbox, that wraps around")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 6",
        spinbox2.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create another spinbox, with a label this time
    
    spinbox3 = vtkKWSpinBoxWithLabel()
    spinbox3.SetParent(parent)
    spinbox3.Create()
    spinbox3.GetWidget().SetRange(10, 100)
    spinbox3.GetWidget().SetIncrement(10)
    spinbox3.SetLabelText("Another spinbox, with a label in front:")
    spinbox3.SetBalloonHelpString(
        "This is a vtkKWSpinBoxWithLabel, i.e. a spinbox associated to a "
        "label that can be positioned around the spinbox.")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 6",
        spinbox3.GetWidgetName())
    
    
    
    return "TypeCore"
