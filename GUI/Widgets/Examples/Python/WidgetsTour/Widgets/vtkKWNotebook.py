from kwwidgets import vtkKWNotebook
from kwwidgets import vtkKWApplication
from kwwidgets import vtkKWWindow
from kwwidgets import vtkKWPushButton
from kwwidgets import vtkKWMessage

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
    notebook2.UseFrameWithScrollbarsOn()
    
    # -----------------------------------------------------------------------
    
    # Create a message inside one of the page (as a test for scrollbars)

    page_id = notebook2.AddPage("Page A")

    lorem_ipsum = "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Nunc felis. Nulla gravida. Aliquam erat volutpat. Mauris accumsan quam non sem. Sed commodo, magna quis bibendum lacinia, elit turpis iaculis augue, eget hendrerit elit dui vel elit.\n\nInteger ante eros, auctor eu, dapibus ac, ultricies vitae, lacus. Fusce accumsan mauris. Morbi felis. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos hymenaeos. Maecenas convallis imperdiet nunc."

    message = vtkKWMessage()
    message.SetParent(notebook2.GetFrame(page_id))
    message.Create()
    message.SetText(lorem_ipsum)
    message.AppendText(lorem_ipsum)
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 2",
        message.GetWidgetName())

    # -----------------------------------------------------------------------

    # Create a disabled page

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
