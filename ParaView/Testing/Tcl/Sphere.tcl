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

# get the source menu
set pvSourceMenu [$pvWindow GetSourceMenu]

# create a sphere
$pvSourceMenu Invoke [$pvSourceMenu GetIndex "SphereSource"]
set pvSphere [$pvWindow GetCurrentPVSource]
$pvSphere AcceptCallback


RenWin1 SetSize 448 603
RenWin1 Render


if {[info exists PARAVIEW_BASELINE]} {
    vtkWindowToImageFilter winToImage
    winToImage SetInput RenWin1
    
    vtkPNGReader reader
    reader SetFileName "${PARAVIEW_BASELINE}/Sphere.png"
    
    vtkImageDifference imgdiff
    imgdiff SetInput [winToImage GetOutput]
    imgdiff SetImage [reader GetOutput]
    imgdiff Update
    
    set threshold 10
    
    set imageError [decipadString [imgdiff GetThresholdedError] 4 9]
    
    if {[imgdiff GetThresholdedError] > $threshold} {
	puts "Failed Image Test with error: $imageError"
	exit 1; 
    } else {
	Application Exit
    }
	
} else {
    puts "A valid image could not be found."
    exit 1;
}



Application Exit


