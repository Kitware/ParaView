from kwwidgets import *


def vtkKWComboBoxEntryPoint(parent, win):

    app = parent.GetApplication()
    
    days = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday"]
    numbers = ["123", "42", "007", "1789"]
    
    # -----------------------------------------------------------------------
    
    # Create a combobox
    
    combobox1 = vtkKWComboBox()
    combobox1.SetParent(parent)
    combobox1.Create()
    combobox1.SetBalloonHelpString("A simple combobox")
    
    for i in range(0,len(days)):
        combobox1.AddValue(days[i])
        
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 2",
        combobox1.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create another combobox, but larger, and read-only
    
    combobox2 = vtkKWComboBox()
    combobox2.SetParent(parent)
    combobox2.Create()
    combobox2.SetWidth(20)
    combobox2.ReadOnlyOn()
    combobox2.SetValue("read-only combobox")
    combobox2.SetBalloonHelpString("Another combobox, larger and read-only")
    
    for i in range(0,len(numbers)):
        combobox2.AddValue(numbers[i])
        
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 6",
        combobox2.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create another combobox, with a label this time
    
    combobox3 = vtkKWComboBoxWithLabel()
    combobox3.SetParent(parent)
    combobox3.Create()
    combobox3.SetLabelText("Another combobox, with a label in front:")
    combobox3.SetBalloonHelpString(
        "This is a vtkKWComboBoxWithLabel, i.e. a combobox associated to a "
        "label that can be positioned around the combobox.")
    
    for i in range(0,len(days)):
        combobox3.GetWidget().AddValue(days[i])
        
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 6",
        combobox3.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create a set of combobox
    # An easy way to create a bunch of related widgets without allocating
    # them one by one
    
    combobox_set = vtkKWComboBoxSet()
    combobox_set.SetParent(parent)
    combobox_set.Create()
    combobox_set.SetBorderWidth(2)
    combobox_set.SetReliefToGroove()
    combobox_set.SetWidgetsPadX(1)
    combobox_set.SetWidgetsPadY(1)
    combobox_set.SetPadX(1)
    combobox_set.SetPadY(1)
    combobox_set.SetMaximumNumberOfWidgetsInPackingDirection(2)
    
    for id in range(0,4):
        combobox = combobox_set.AddWidget(id)
        combobox.SetBalloonHelpString(
            "This combobox is part of a unique set (a vtkKWComboBoxSet), "
            "which provides an easy way to create a bunch of related widgets "
            "without allocating them one by one. The widgets can be layout as a "
            "NxM grid.")
        
        for i in range(0,len(days)):
            combobox.AddValue(days[i])
            
        
    
    # Let's be creative. The first one sets the value of the third one
    
    combobox_set.GetWidget(0).SetValue("Enter a value here...")
    combobox_set.GetWidget(2).SetValue("...and it will show here.")
    combobox_set.GetWidget(2).DeleteAllValues()
    
    combobox_set.GetWidget(0).SetCommand(
        combobox_set.GetWidget(2), "SetValue")
    
    # Let's be creative. The second one adds its value to the fourth one
    
    combobox_set.GetWidget(1).SetValue("Enter a value here...")
    combobox_set.GetWidget(3).SetValue("...and it will be added here.")
    combobox_set.GetWidget(3).DeleteAllValues()
    
    combobox_set.GetWidget(1).SetCommand(
        combobox_set.GetWidget(3), "AddValue")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 6",
        combobox_set.GetWidgetName())
    
    
    
    return "TypeCore"
