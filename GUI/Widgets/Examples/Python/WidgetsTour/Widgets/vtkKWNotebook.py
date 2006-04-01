from kwwidgets import vtkKWNotebook
from kwwidgets import vtkKWApplication
from kwwidgets import vtkKWWindow
from kwwidgets import vtkKWPushButton

def vtkKWNotebookEntryPoint(parent, win):

    app = parent.GetApplication()
    
    # -----------------------------------------------------------------------
    
    # Create a notebook
    
    notebook1 = vtkKWNotebook()
    notebook1.SetParent(parent)
    notebook1.SetMinimumWidth(400)
    notebook1.SetMinimumHeight(200)
    notebook1.Create()
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 2",
        notebook1.GetWidgetName())
    
    # Add some pages
    
    notebook1.AddPage("Page 1")
    
    notebook1.AddPage("Page Blue")
    notebook1.GetFrame("Page Blue").SetBackgroundColor(0.2, 0.2, 0.9)
    
    page_id = notebook1.AddPage("Page Red")
    notebook1.GetFrame(page_id).SetBackgroundColor(0.9, 0.2, 0.2)
    
    # -----------------------------------------------------------------------

    # Create a notebook inside one of the page (because we can)

    page_id = notebook1.AddPage("Sub Notebook")

    notebook2 = vtkKWNotebook()
    notebook2.SetParent(notebook1.GetFrame(page_id))
    notebook2.Create()
    notebook2.EnablePageTabContextMenuOn()
    notebook2.PagesCanBePinnedOn()
    
    notebook2.AddPage("Page A")

    page_id = notebook2.AddPage("Page Disabled")
    notebook2.SetPageEnabled(page_id, 0)
    
    app.Script(
        "pack %s -side top -anchor nw -expand y -fill both -padx 2 -pady 2", 
        notebook2.GetWidgetName())
    
    # -----------------------------------------------------------------------

    # Create a button inside one of the page (as a test)

    page_id = notebook2.AddPage("Button Page")
    
    pushbutton1 = vtkKWPushButton()
    pushbutton1.SetParent(notebook2.GetFrame(page_id))
    pushbutton1.Create()
    pushbutton1.SetText("A push button")
    
    app.Script("pack %s -side top -anchor c -expand y", 
                pushbutton1.GetWidgetName())
    
    return "TypeComposite"
