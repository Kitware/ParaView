from kwwidgets import *


def vtkKWListBoxToListBoxSelectionEditorEntryPoint(parent, win):

    app = parent.GetApplication()
    
    # -----------------------------------------------------------------------
    
    # Create a list box to list box selection editor
    
    lb2lb1 = vtkKWListBoxToListBoxSelectionEditor()
    lb2lb1.SetParent(parent)
    lb2lb1.Create()
    lb2lb1.SetReliefToGroove()
    lb2lb1.SetBorderWidth(2)
    lb2lb1.SetPadX(2)
    lb2lb1.SetPadY(2)
    
    lb2lb1.AddSourceElement("Monday", 0)
    lb2lb1.AddSourceElement("Tuesday", 0)
    lb2lb1.AddSourceElement("Wednesday", 0)
    lb2lb1.AddFinalElement("Thursday", 0)
    lb2lb1.AddFinalElement("Friday", 0)
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 2",
        lb2lb1.GetWidgetName())
    
    
    
    return "TypeComposite"
