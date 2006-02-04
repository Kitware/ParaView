from kwwidgets import vtkKWFrame
from kwwidgets import vtkKWFrameWithLabel
from kwwidgets import vtkKWApplication
from kwwidgets import vtkKWWindow



def vtkKWFrameEntryPoint(parent, win):

    app = parent.GetApplication()
    
    # -----------------------------------------------------------------------
    
    # Create a frame
    
    frame1 = vtkKWFrame()
    frame1.SetParent(parent)
    frame1.Create()
    frame1.SetWidth(200)
    frame1.SetHeight(50)
    frame1.SetBackgroundColor(0.5, 0.5, 0.95)
    frame1.SetBalloonHelpString(
        "Another frame, set its size explicitly and change its color")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 6",
        frame1.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create a labeled frame
    
    frame2 = vtkKWFrameWithLabel()
    frame2.SetParent(parent)
    frame2.Create()
    frame2.SetLabelText("A Labeled Frame")
    frame2.SetWidth(300)
    frame2.SetHeight(100)
    frame2.SetBalloonHelpString(
        "This is a vtkKWFrameWithLabel, i.e. a frame associated to a "
        "label on top of it, that can be collapsed or expanded. Its size is"
        "set explicitly here, but should adjust automatically otherwise")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 6",
        frame2.GetWidgetName())
    
    
    
    return "TypeCore"
