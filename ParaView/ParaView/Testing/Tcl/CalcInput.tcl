    if {[info exists argc]} { 
        set argcm1 [expr $argc - 1]
        for {set i 0} {$i < $argcm1} {incr i} {
            if {[lindex $argv $i] == "-D" && $i < $argcm1} {
		set DataDir [lindex $argv [expr $i + 1]]
            }
        }
    } 


# I ran into a bug where the input to the calculator could not be set,
# (this was the trace).  I am adding the test to make sure it does 
# not happen again.

set pv(vtkTemp1) [Application GetMainWindow]
set pv(vtkTemp43) [$pv(vtkTemp1) GetMainView]
set pv(vtkTemp89) [$pv(vtkTemp1) GetGlyphSource Arrow]
set pv(vtkTemp93) [$pv(vtkTemp1) GetGlyphSource Cone]
set pv(vtkTemp97) [$pv(vtkTemp1) GetGlyphSource Sphere]

set pv(vtkTemp98) [$pv(vtkTemp1) CreatePVSource vtkSphereSource]
$pv(vtkTemp98) AcceptCallback

set pv(vtkTemp182) [$pv(vtkTemp1) CalculatorCallback]
$pv(vtkTemp182) AddScalarVariable Normals_0 Normals 0
$pv(vtkTemp182) AddScalarVariable Normals_1 Normals 1
$pv(vtkTemp182) AddScalarVariable Normals_2 Normals 2
$pv(vtkTemp182) SetFunctionLabel {Normals_0+Normals_1+Normals_2}
$pv(vtkTemp182) AcceptCallback

set pv(vtkTemp211) [$pv(vtkTemp1) CreatePVSource vtkCylinderSource]
set pv(vtkTemp232) [$pv(vtkTemp211) GetPVWidget {Center}]
$pv(vtkTemp232) SetValue 1 0 0
set pv(vtkTemp233) [$pv(vtkTemp211) GetPVWidget {Resolution}]
$pv(vtkTemp233) SetValue 12
$pv(vtkTemp211) AcceptCallback
$pv(vtkTemp1) ResetCameraCallback

set pv(vtkTemp247) [$pv(vtkTemp1) CreatePVSource vtkShrinkPolyData]
$pv(vtkTemp247) AcceptCallback

$pv(vtkTemp1) SetCurrentPVSource $pv(vtkTemp182)
$pv(vtkTemp1) ShowCurrentSourceProperties
set pv(vtkTemp190) [$pv(vtkTemp182) GetPVWidget {Input}]
$pv(vtkTemp190) SetCurrentValue $pv(vtkTemp247)
$pv(vtkTemp182) AcceptCallback

# Fix the size of the image.
RenWin1 SetSize 300 300
RenWin1 Render
update

source $DataDir/Utility/rtImage.tcl
pvImageTest $DataDir/Baseline/CalcInput.png 10
   
Application Exit


