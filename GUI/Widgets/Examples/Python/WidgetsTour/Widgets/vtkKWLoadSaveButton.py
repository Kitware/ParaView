from kwwidgets import *


def vtkKWLoadSaveButtonEntryPoint(parent, win):

    app = parent.GetApplication()
    
    # -----------------------------------------------------------------------
    
    # Create a load button
    
    load_button1 = vtkKWLoadSaveButton()
    load_button1.SetParent(parent)
    load_button1.Create()
    load_button1.SetText("Click to Pick a File")
    load_button1.GetLoadSaveDialog().SaveDialogOff()# load mode
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 2",
        load_button1.GetWidgetName())
    
    
    
    return "TypeComposite"
