from kwwidgets import vtkKWProgressGauge
from kwwidgets import vtkKWPushButtonSet
from kwwidgets import vtkKWPushButton
from kwwidgets import vtkKWApplication
from kwwidgets import vtkKWWindow



def vtkKWProgressGaugeEntryPoint(parent, win):

    app = parent.GetApplication()
    
    # -----------------------------------------------------------------------
    
    # Create a progress gauge
    
    progress1 = vtkKWProgressGauge()
    progress1.SetParent(parent)
    progress1.Create()
    progress1.SetWidth(150)
    progress1.SetBorderWidth(2)
    progress1.SetReliefToGroove()
    progress1.SetPadX(2)
    progress1.SetPadY(2)
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 2",
        progress1.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create a set of pushbutton that will modify the progress gauge
    
    progress1_pbs = vtkKWPushButtonSet()
    progress1_pbs.SetParent(parent)
    progress1_pbs.Create()
    progress1_pbs.SetBorderWidth(2)
    progress1_pbs.SetReliefToGroove()
    progress1_pbs.SetWidgetsPadX(1)
    progress1_pbs.SetWidgetsPadY(1)
    progress1_pbs.SetPadX(1)
    progress1_pbs.SetPadY(1)
    progress1_pbs.ExpandWidgetsOn()
    
    for id in range(0,100,25):
        buffer = "Set Progress to %d%%" % (id)
        pushbutton = progress1_pbs.AddWidget(id)
        pushbutton.SetText(buffer)
        buffer = "SetValue %d" % (id)
        pushbutton.SetCommand(progress1, buffer)
    
    # Add a special button that will iterate from 0 to 100% in Tcl
    
    pushbutton = progress1_pbs.AddWidget(1000)
    pushbutton.SetText("0% to 100%")
    
    buffer = "for {set i 0} {$i <= 100} {incr i} { %s SetValue $i ; after 20; update}" % (progress1.GetTclName())
    pushbutton.SetCommand(None, buffer)

    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 6",
        progress1_pbs.GetWidgetName())


    # TODO: add callbacks

    return "TypeComposite"
