from kwwidgets import vtkKWListBox
from kwwidgets import vtkKWListBoxWithScrollbars
from kwwidgets import vtkKWApplication
from kwwidgets import vtkKWWindow



def vtkKWListBoxEntryPoint(parent, win):

    app = parent.GetApplication()
    
    days = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday"]
    
    # -----------------------------------------------------------------------
    
    # Create a listbox
    # Add some entries
    
    listbox1 = vtkKWListBox()
    listbox1.SetParent(parent)
    listbox1.Create()
    listbox1.SetSelectionModeToSingle()
    listbox1.SetBalloonHelpString("A simple listbox")
    
    for i in range(0,len(days)):
        listbox1.AppendUnique(days[i])
        
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 2",
        listbox1.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create another listbox, this time with scrollbars and multiple choices
    
    listbox2 = vtkKWListBoxWithScrollbars()
    listbox2.SetParent(parent)
    listbox2.Create()
    listbox2.SetBorderWidth(2)
    listbox2.SetReliefToGroove()
    listbox2.SetPadX(2)
    listbox2.SetPadY(2)
    listbox2.GetWidget().SetSelectionModeToMultiple()
    listbox2.GetWidget().ExportSelectionOff()
    listbox2.SetBalloonHelpString(
        "A list box with scrollbars. Multiple entries can be selected")
    
    for i in range(0,15):
        buffer = "Entry %d" % int(i)
        listbox2.GetWidget().AppendUnique(buffer)
            
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 6",
        listbox2.GetWidgetName())
    
    
    # TODO: use callbacks
    
    
    return "TypeCore"
