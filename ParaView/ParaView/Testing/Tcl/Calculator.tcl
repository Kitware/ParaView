    if {[info exists argc]} { 
        set argcm1 [expr $argc - 1]
        for {set i 0} {$i < $argcm1} {incr i} {
            if {[lindex $argv $i] == "-D" && $i < $argcm1} {
		set DataDir [lindex $argv [expr $i + 1]]
            }
        }
    } 

set pv(vtkTemp1) [Application GetMainWindow]
set pv(vtkTemp43) [$pv(vtkTemp1) GetMainView]
set pv(vtkTemp89) [$pv(vtkTemp1) GetGlyphSource Arrow]
set pv(vtkTemp93) [$pv(vtkTemp1) GetGlyphSource Cone]
set pv(vtkTemp97) [$pv(vtkTemp1) GetGlyphSource Sphere]
set pv(vtkTemp100) [$pv(vtkTemp1) Open "$DataDir/Data/blow.vtk"]
set pv(vtkTemp156) [$pv(vtkTemp1) CalculatorCallback]
$pv(vtkTemp156) AddVectorVariable displacement6 displacement6
$pv(vtkTemp156) AddScalarVariable thickness6 thickness6 0
$pv(vtkTemp156) SetFunctionLabel {mag(displacement6)*thickness6}
set pv(vtkTemp168) [$pv(vtkTemp156) GetPVWidget {Result Array Name:}]
$pv(vtkTemp168) SetValue {outputArray}
$pv(vtkTemp156) AcceptCallback

# Fix the size of the image.
RenWin1 SetSize 300 300
RenWin1 Render
update

source $DataDir/Utility/rtImage.tcl
pvImageTest $DataDir/Baseline/Calculator.png 10
   
Application Exit
