#package require vtk
#package require vtkinteraction

wm withdraw .


# get the interactor ui

vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
  renWin SetSize 800 800
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin
ren1 SetBackground .1 .2 .4

set fp [open "timeMapper.txt" w]

set numSpheres 1
set Iterations 1000

proc reportTime {} {
    global normals cellNormals scalars cellScalars strips fp numSpheres
    global Iterations
    # create some points with scalars

    for {set i 1} {$i <= $numSpheres} {incr i} {
      vtkSphereSource sphere$i
        sphere$i SetRadius [expr $i + 2]
        sphere$i SetThetaResolution 200
        sphere$i SetPhiResolution 100
        sphere$i Update 

      vtkElevationFilter elevation$i
        elevation$i SetLowPoint 0 0 [expr -($numSpheres+2)]
        elevation$i SetHighPoint 0 0 [expr $numSpheres+2]
        elevation$i SetInput [sphere$i GetOutput]

      vtkPointDataToCellData p2c$i
        p2c$i SetInput [elevation$i GetOutput]
        p2c$i PassPointDataOn
        p2c$i Update

      vtkPolyData tmp$i
        tmp$i ShallowCopy [p2c$i GetOutput]
      if {$normals == 0} {  
        [tmp$i GetPointData] SetNormals {}
      }
      if {$cellNormals == 0} {  
        [tmp$i GetCellData] SetNormals {}
      }
      if {$scalars == 0} {  
        [tmp$i GetPointData] SetScalars {}
      }
      if {$cellScalars == 0} {  
        [tmp$i GetCellData] SetScalars {}
      }

      sphere$i Delete
      elevation$i Delete
      p2c$i Delete

      vtkPolyDataMapper mapper$i
      mapper$i SetInput tmp$i
      mapper$i ImmediateModeRenderingOn
      tmp$i Delete

      vtkActor actor$i
        actor$i SetMapper mapper$i
      mapper$i Delete      

     ren1 AddActor actor$i
     actor$i Delete
   }

   renWin Render
   ren1 ResetCamera
   [ren1 GetActiveCamera] Dolly 1.5
   ren1 ResetCameraClippingRange

   set t [time {
     for {set i 0} {$i < $Iterations} {incr i} {
	[ren1 GetActiveCamera] Azimuth 1
	renWin Render
     }
   }]

  puts $fp "PointScalars: $scalars CellScalars: $cellScalars "
  puts $fp "PointNormals: $normals CellNormals: $cellNormals "
  puts $fp "Time = $t"
  puts $fp ""

  ren1 RemoveAllProps
}


# testing trinagles and strips.


# Normals with no coloring
# I believe this was optimized.
set normals      1
set cellNormals  0
set scalars      0
set cellScalars  0
set strips       0
# time: 67.5 Seconds.
# time 55.4 Seconds.
# macro: 67.9
reportTime

# Flat shading without cell normals.
# Do not bother optimizing this.
set normals      0
set cellNormals  0
set scalars      0
set cellScalars  0
set strips       0
# time: 101.3 Seconds.
# time: 99.3 (Call normal once of each trinagle)
# time: 93.9 (Cell Array pointer)
# Macro: 140.6
reportTime

# flat shading without normals.
# with point colors.
# Do not bother optimizing this.
set normals      0
set cellNormals  0
set scalars      1
set cellScalars  0
set strips       0
# time: 135.1 Seconds.
# time: 126   Seconds. (Cache colors).
# Macro: 153
reportTime

# Optimized
# Flat shading with cell normals (and point scalars).
# compare this to flat shading without normals (and point scalars).
set normals      0
set cellNormals  1
set scalars      1
set cellScalars  0
set strips       0
# time: 113.3
# time: 68.8 Optimized.
# macro: 197
reportTime

# Optimized
# Flat shading with cell normals ( no color).
# compare this to flat shading without normals.
set normals      0
set cellNormals  1
set scalars      0
set cellScalars  0
set strips       0
# time: 75.7
# time: 51.6
# Macro: 136.3
reportTime

# optimize this.
# smooth shading with cell colors.
set normals      1
set cellNormals  0
set scalars      0
set cellScalars  1
set strips       0
# time: 119.8
# Macro: 202.4
reportTime

# optimize this.
# flat shading (cell normals) with cell colors.
set normals      0
set cellNormals  1
set scalars      0
set cellScalars  1
set strips       0
# time: 103 seconds
# time: 68.8
# Macro: 202.
reportTime

# Normals and Colors
set normals       1
set cellNormals   0
set scalars       1
set cellScalars   0
set strips        0
# time: 101.6 Seconds.
# time: 100.8 Seconds. (Cell Array Pointer ????)
# time: 85.9  Seconds. (Cache the colors).
# final 68.8 Seconds
# Macro: 85.8
reportTime


close $fp

exit








