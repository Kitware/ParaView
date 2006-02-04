from kwwidgets import vtkKWSplitFrame
from kwwidgets import vtkKWApplication
from kwwidgets import vtkKWWindow



def vtkKWSplitFrameEntryPoint(parent, win):

    app = parent.GetApplication()
    
    # -----------------------------------------------------------------------
    
    # Create a split frame
    
    splitframe1 = vtkKWSplitFrame()
    splitframe1.SetParent(parent)
    splitframe1.Create()
    splitframe1.SetWidth(400)
    splitframe1.SetHeight(200)
    splitframe1.SetReliefToGroove()
    splitframe1.SetBorderWidth(2)
    splitframe1.SetExpandableFrameToBothFrames()
    splitframe1.SetFrame1MinimumSize(5)
    splitframe1.SetFrame2MinimumSize(5)
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 2",
        splitframe1.GetWidgetName())
    
    # Change the color of each pane
    
    splitframe1.GetFrame1().SetBackgroundColor(0.2, 0.2, 0.95)
    splitframe1.GetFrame2().SetBackgroundColor(0.95, 0.2, 0.2)
    
    
    
    return "TypeComposite"
