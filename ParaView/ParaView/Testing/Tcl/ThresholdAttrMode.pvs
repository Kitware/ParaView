    if {[info exists argc]} { 
        set argcm1 [expr $argc - 1]
        for {set i 0} {$i < $argcm1} {incr i} {
            if {[lindex $argv $i] == "-D" && $i < $argcm1} {
		set DataDir [lindex $argv [expr $i + 1]]
            }
        }
    } 


set pv(vtkTemp1) [Application GetMainWindow]
set pv(vtkTemp44) [$pv(vtkTemp1) GetMainView]
set pv(vtkTemp90) [$pv(vtkTemp1) GetGlyphSource Arrow]
set pv(vtkTemp94) [$pv(vtkTemp1) GetGlyphSource Cone]
set pv(vtkTemp98) [$pv(vtkTemp1) GetGlyphSource Sphere]
set pv(vtkTemp101) [$pv(vtkTemp1) Open $DataDir/Data/vox8_bin.vtk]
set pv(vtkTemp129) [$pv(vtkTemp1) ThresholdCallback]
set pv(vtkTemp137) [$pv(vtkTemp129) GetAttributeModeMenu]
$pv(vtkTemp137) SetValue "Cell Data"
$pv(vtkTemp129) ChangeAttributeMode cell
set pv(vtkTemp135) [$pv(vtkTemp129) GetPVWidget {Input}]
$pv(vtkTemp135) SetCurrentValue $pv(vtkTemp101)
set pv(vtkTemp141) [$pv(vtkTemp129) GetPVWidget {Lower Threshold}]
$pv(vtkTemp141) SetMinValue 0.048000
$pv(vtkTemp141) SetMaxValue 0.400000
set pv(vtkTemp146) [$pv(vtkTemp129) GetPVWidget {AllScalars}]
$pv(vtkTemp129) AcceptCallback
set pv(vtkTemp158) [$pv(vtkTemp1) CreatePVSource vtkDataSetSurfaceFilter]
set pv(vtkTemp164) [$pv(vtkTemp158) GetPVWidget {Input}]
$pv(vtkTemp164) SetCurrentValue $pv(vtkTemp129)
$pv(vtkTemp158) AcceptCallback

# Fix the size of the image.
RenWin1 SetSize 300 300
RenWin1 Render
update

source $DataDir/Utility/rtImage.tcl
pvImageTest $DataDir/Baseline/ThresholdAttrMode.png 10
   
Application Exit
