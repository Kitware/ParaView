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

set pv(vtkTemp98) [$pv(vtkTemp1) CreatePVSource vtkSphereSource]
$pv(vtkTemp1) SetCurrentPVSource $pv(vtkTemp98)
$pv(vtkTemp1) ShowCurrentSourceProperties
$pv(vtkTemp98) AcceptCallback

set pv(vtkTemp183) [$pv(vtkTemp1) GlyphCallback]
$pv(vtkTemp1) SetCurrentPVSource $pv(vtkTemp183)
$pv(vtkTemp1) ShowCurrentSourceProperties
set pv(vtkTemp204) [$pv(vtkTemp183) GetPVWidget {Scale Factor}]
$pv(vtkTemp204) SetValue 0.2
$pv(vtkTemp183) AcceptCallback

set pv(vtkTemp217) [$pv(vtkTemp1) CreatePVSource vtkSphereSource]
$pv(vtkTemp1) SetCurrentPVSource $pv(vtkTemp217)
$pv(vtkTemp1) ShowCurrentSourceProperties
set pv(vtkTemp229) [$pv(vtkTemp217) GetPVWidget {Center}]
$pv(vtkTemp229) SetValue 1.5 0 0
$pv(vtkTemp217) AcceptCallback
$pv(vtkTemp1) ResetCameraCallback
set pv(vtkTemp262) [$pv(vtkTemp1) GlyphCallback]
$pv(vtkTemp1) SetCurrentPVSource $pv(vtkTemp262)
$pv(vtkTemp1) ShowCurrentSourceProperties
set pv(vtkTemp271) [$pv(vtkTemp262) GetPVWidget {Glyph}]
$pv(vtkTemp271) SetCurrentValue $pv(vtkTemp93)
set pv(vtkTemp283) [$pv(vtkTemp262) GetPVWidget {Scale Factor}]
$pv(vtkTemp283) SetValue 0.2
set pv(vtkTemp285) [$pv(vtkTemp262) GetPVWidget {Vector Mode}]
$pv(vtkTemp285) SetCurrentValue {1}
$pv(vtkTemp262) AcceptCallback

# Test deleting before accept is called.
#set pv(vtkTemp296) [$pv(vtkTemp1) CreatePVSource vtkSphereSource]
#$pv(vtkTemp1) SetCurrentPVSource $pv(vtkTemp296)
#$pv(vtkTemp1) ShowCurrentSourceProperties
#$pv(vtkTemp296) DeleteCallback
#$pv(vtkTemp1) SetCurrentPVSource $pv(vtkTemp262)
#$pv(vtkTemp1) ShowCurrentSourceProperties

set pv(vtkTemp333) [$pv(vtkTemp1) CreatePVSource vtkSphereSource]
$pv(vtkTemp1) SetCurrentPVSource $pv(vtkTemp333)
$pv(vtkTemp1) ShowCurrentSourceProperties
set pv(vtkTemp345) [$pv(vtkTemp333) GetPVWidget {Center}]
$pv(vtkTemp345) SetValue 0 1.5 0
$pv(vtkTemp333) AcceptCallback
set pv(vtkTemp378) [$pv(vtkTemp1) GlyphCallback]
$pv(vtkTemp1) SetCurrentPVSource $pv(vtkTemp378)
$pv(vtkTemp1) ShowCurrentSourceProperties
set pv(vtkTemp399) [$pv(vtkTemp378) GetPVWidget {Scale Factor}]
$pv(vtkTemp399) SetValue 1
$pv(vtkTemp378) AcceptCallback
set pv(vtkTemp387) [$pv(vtkTemp378) GetPVWidget {Glyph}]
$pv(vtkTemp387) SetCurrentValue $pv(vtkTemp97)
$pv(vtkTemp399) SetValue 0.1
$pv(vtkTemp378) AcceptCallback

set pv(vtkTemp412) [$pv(vtkTemp1) CreatePVSource vtkSphereSource]
$pv(vtkTemp1) SetCurrentPVSource $pv(vtkTemp412)
$pv(vtkTemp1) ShowCurrentSourceProperties
set pv(vtkTemp424) [$pv(vtkTemp412) GetPVWidget {Center}]
$pv(vtkTemp424) SetValue 1.5 1.5 0
$pv(vtkTemp412) AcceptCallback
$pv(vtkTemp1) ResetCameraCallback
$pv(vtkTemp1) SetCurrentPVSource $pv(vtkTemp183)
$pv(vtkTemp1) ShowCurrentSourceProperties
set pv(vtkTemp191) [$pv(vtkTemp183) GetPVWidget {Input}]
$pv(vtkTemp191) SetCurrentValue $pv(vtkTemp412)
$pv(vtkTemp204) SetValue 0.2
$pv(vtkTemp183) AcceptCallback
$pv(vtkTemp183) AcceptCallback
$pv(vtkTemp1) SetCurrentPVSource $pv(vtkTemp98)
$pv(vtkTemp1) ShowCurrentSourceProperties
set pv(vtkTemp458) [$pv(vtkTemp1) GlyphCallback]
$pv(vtkTemp1) SetCurrentPVSource $pv(vtkTemp458)
$pv(vtkTemp1) ShowCurrentSourceProperties
set pv(vtkTemp479) [$pv(vtkTemp458) GetPVWidget {Scale Factor}]
$pv(vtkTemp479) SetValue 0.4
set pv(vtkTemp482) [$pv(vtkTemp458) GetPVWidget {Vector Mode}]
$pv(vtkTemp482) SetCurrentValue {1}


$pv(vtkTemp458) AcceptCallback

# Fix the size of the image.
RenWin1 SetSize 300 300
RenWin1 Render
update

source $DataDir/Utility/rtImage.tcl
pvImageTest $DataDir/Baseline/Glyph.png 10
   
Application Exit








