catch {set PARAVIEW_BASELINE $env(PARAVIEW_BASELINE)}

proc decipadString { str before total } {
    set x [string first "." $str]
    if { $x == -1 } { 
	set str "${str}.0"
    }

    set x [string first "." $str]
    while { $x >= 0 && $x < $before } {
	set str " $str"
	set x [string first "." $str]
    }

    if { [string length $str] >= $total } {
        return [string range $str 0 [expr $total - 1]]
    }

    while { [string length $str] < $total } {
        set str "${str}0"
    }
    return $str
}

# hack to get the window.
set pvWindow [lindex [vtkPVWindow ListInstances] 0]

set sInt [$pvWindow GetSourceInterface "vtkSphereSource"]
set pvSphere [$sInt CreateCallback]
[$pvSphere GetVTKSource] SetThetaResolution 16
$pvSphere UpdateParameterWidgets
$pvSphere AcceptCallback

set sInt [$pvWindow GetSourceInterface "vtkShrinkPolyData"]
set pvShrink [$sInt CreateCallback]
[$pvShrink GetVTKSource] SetShrinkFactor 0.9
$pvShrink UpdateParameterWidgets
$pvShrink AcceptCallback


$pvShrink DeleteCallBack


RenWin1 SetSize 250 250
RenWin1 Render
Application Exit


