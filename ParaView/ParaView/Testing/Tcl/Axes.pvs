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
set pv(vtkTemp98) [$pv(vtkTemp1) CreatePVSource vtkAxes]
$pv(vtkTemp1) SetCurrentPVSource $pv(vtkTemp98)
$pv(vtkTemp1) ShowCurrentSourceProperties
$pv(vtkTemp98) AcceptCallback
[$pv(vtkTemp98) GetPVOutput 0] SetLineWidth 3
set pv(vtkTemp168) [$pv(vtkTemp1) CreatePVSource vtkAxes]
$pv(vtkTemp1) SetCurrentPVSource $pv(vtkTemp168)
$pv(vtkTemp1) ShowCurrentSourceProperties
set pv(vtkTemp183) [$pv(vtkTemp168) GetPVWidget {Origin}]
$pv(vtkTemp183) SetValue 0 3 0
set pv(vtkTemp188) [$pv(vtkTemp168) GetPVWidget {Symmetric}]
$pv(vtkTemp188) SetState 1
$pv(vtkTemp168) AcceptCallback
[$pv(vtkTemp168) GetPVOutput 0] SetLineWidth 3
$pv(vtkTemp1) ResetCameraCallback
$pv(vtkTemp1) SetCurrentPVSource $pv(vtkTemp168)
$pv(vtkTemp1) ShowCurrentSourceProperties
$pv(vtkTemp1) SetCurrentPVSource $pv(vtkTemp98)
$pv(vtkTemp1) ShowCurrentSourceProperties

# Position the camera
set camera [Ren1 GetActiveCamera]
$camera  SetPosition 7.96507 9.75632 6.90566
$camera SetFocalPoint 0.190213 2.17507 -0.136077
$camera SetViewUp 0.252184 0.506968 -0.824249
$camera SetViewAngle 30
$camera SetClippingRange 8.34501 19.084

# Fix the size of the image.
RenWin1 SetSize 300 300
RenWin1 Render
update

source $DataDir/Utility/rtImage.tcl
pvImageTest $DataDir/Baseline/Axes.png 10

Application Exit
