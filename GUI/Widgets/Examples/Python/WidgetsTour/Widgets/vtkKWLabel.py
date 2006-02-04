from kwwidgets import vtkKWLabel
from kwwidgets import vtkKWLabelWithLabel
from kwwidgets import vtkKWLabelSet
from kwwidgets import vtkKWLabelWithLabelSet
from kwwidgets import vtkKWApplication
from kwwidgets import vtkKWWindow
from kwwidgets import vtkKWIcon
from vtk import vtkMath



def vtkKWLabelEntryPoint(parent, win):

    app = parent.GetApplication()
    
    # -----------------------------------------------------------------------
    
    # Create a label
    
    label1 = vtkKWLabel()
    label1.SetParent(parent)
    label1.Create()
    label1.SetText("A label")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 2",
        label1.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create another label, right justify it
    
    label2 = vtkKWLabel()
    label2.SetParent(parent)
    label2.Create()
    label2.SetText("Another label")
    label2.SetJustificationToRight()
    label2.SetWidth(30)
    label2.SetBackgroundColor(0.5, 0.5, 0.95)
    label2.SetBorderWidth(2)
    label2.SetReliefToGroove()
    label2.SetBalloonHelpString(
        "Another label, explicit width, right-justified")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 6",
        label2.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create another label, with a label this time (!)
    
    label3 = vtkKWLabelWithLabel()
    label3.SetParent(parent)
    label3.Create()
    label3.GetLabel().SetText("Name:")
    label3.GetLabel().SetBackgroundColor(0.7, 0.7, 0.7)
    label3.GetWidget().SetText("Sebastien Barre")
    label3.SetBalloonHelpString(
        "This is a vtkKWLabelWithLabel, i.e. a label associated to a "
        "label that can be positioned around the label. This can be used for "
        "example to label a value without having to construct a single "
        "label out of two separate elements, one of them likely not to change.")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 6",
        label3.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create another label, with a label this time (!)
    
    label4 = vtkKWLabelWithLabel()
    label4.SetParent(parent)
    label4.Create()
    label4.GetLabel().SetImageToPredefinedIcon(vtkKWIcon.IconInfoMini)
    label4.GetWidget().SetText("Another use of a labeled label !")
    label4.SetBalloonHelpString(
        "This is a vtkKWLabelWithLabel, i.e. a label associated to a "
        "label that can be positioned around the label. This can be used for "
        "example to prefix a label with a small icon to emphasize its meaning. "
        "Predefined icons include warning, info, error, etc.")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 6",
        label4.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create a set of label
    # An easy way to create a bunch of related widgets without allocating
    # them one by one
    
    label_set = vtkKWLabelSet()
    label_set.SetParent(parent)
    label_set.Create()
    label_set.SetBorderWidth(2)
    label_set.SetReliefToGroove()
    label_set.SetMaximumNumberOfWidgetsInPackingDirection(3)
    label_set.ExpandWidgetsOn()
    label_set.SetWidgetsPadX(1)
    label_set.SetWidgetsPadY(1)
    label_set.SetPadX(1)
    label_set.SetPadY(1)
    
    for id in range(0,9):
        buffer = "Label %d" % (id)
        label = label_set.AddWidget(id)
        label.SetText(buffer)
        label.SetBackgroundColor(vtkMath.HSVToRGB(float(id) / 8.0, 0.3, 0.75))
        label.SetBalloonHelpString(
            "This label is part of a unique set (a vtkKWLabelSet), "
            "which provides an easy way to create a bunch of related widgets "
            "without allocating them one by one. The widgets can be layout as a "
            "NxM grid.")
        
    
    label_set.GetWidget(0).SetText("Firs Label")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 6",
        label_set.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Even trickier: create a set of labeled label !
    # An easy way to create a bunch of related widgets without allocating
    # them one by one
    
    label_set2 = vtkKWLabelWithLabelSet()
    label_set2.SetParent(parent)
    label_set2.Create()
    label_set2.SetBorderWidth(2)
    label_set2.SetReliefToGroove()
    label_set2.SetWidgetsPadX(1)
    label_set2.SetWidgetsPadY(1)
    label_set2.SetPadX(1)
    label_set2.SetPadY(1)
    
    for id in range(0,3):
        label = label_set2.AddWidget(id)
        label.SetLabelWidth(15)
        label.GetLabel().SetBackgroundColor(0.7, 0.7, 0.7)
        label.SetBalloonHelpString(
            "This labeled label (!) is part of a unique set "
            "(a vtkKWWithLabelLabelSet).")
        
    
    label_set2.GetWidget(0).SetLabelText("First Name:")
    label_set2.GetWidget(0).GetWidget().SetText("Sebastien")
    label_set2.GetWidget(1).SetLabelText("Name:")
    label_set2.GetWidget(1).GetWidget().SetText("Barre")
    label_set2.GetWidget(2).SetLabelText("Company:")
    label_set2.GetWidget(2).GetWidget().SetText("Kitware, Inc.")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 6",
        label_set2.GetWidgetName())
    
    
    
    return "TypeCore"
