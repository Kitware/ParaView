
set RotationStep 5
set DepthStep 9
set IsoStep 0.001



# ------------------  Loop through depth showing salinity -------------

$pvExtractGrid SetVisibility 1
$pvWindow SetCurrentPVSource $pvExtractGrid
update

# Setup color map to show Salinity.
[$pvExtractGridOutput GetColorMenu] SetValue {Point Salinity}
$pvExtractGridOutput ColorByPointFieldComponent Salinity 0
[$pvExtractGridOutput GetColorMapMenu] SetValue {Red to Blue}
$pvExtractGridOutput ChangeColorMap
$pvExtractGridOutput SetScalarRange 0.03 0.04

# Change the color of the continents to match the scalar range.
$pvContOutput ChangeActorColor 1 0 0


# Loop over all of the depth values.
for {set i 1} {$i < 30} {set i [expr $i + $DepthStep]} {
    puts "Salt depth $i"
    [$pvExtractGrid GetVTKSource] SetVOI 0 360 0 239 $i $i
    $pvExtractGrid UpdateParameterWidgets
    $pvExtractGrid AcceptCallback
}

# ------------------  Loop through depth showing temperature -------------

$pvWindow SetCurrentPVSource $pvExtractGrid
$pvExtractGrid SetVisibility 1

# Setup color map to show temperature.
set pvExtractGridOutput [$pvExtractGrid GetNthPVOutput 0]
[$pvExtractGridOutput GetColorMenu] SetValue {Point Temperature}
$pvExtractGridOutput ColorByPointFieldComponent Temperature 0
[$pvExtractGridOutput GetColorMapMenu] SetValue {Blue to Red}
$pvExtractGridOutput ChangeColorMap
$pvExtractGridOutput SetScalarRange -5.5 30.8804

# Turn on the cube-axis actor.
$pvExtractGridOutput SetCubeAxesVisibility 1

# Turn on scalar bar.
$pvExtractGridOutput SetScalarBarVisibility 1
$pvExtractGridOutput SetScalarBarOrientationToHorizontal

# Turn on the cube axes.
$pvExtractGridOutput SetCubeAxesVisibility 0

# set the continent color to match the Temperature of 0.
#$pvContOutput ChangeActorColor 0 [expr 175.0/255.0] 1.0
$pvContOutput ChangeActorColor 0 [expr 170.0/255.0] 1.0


# Loop over all of the depth values.
for {set i 1} {$i < 30} {set i [expr $i + $DepthStep]} {
    puts "Temperature depth $i"
    [$pvExtractGrid GetVTKSource] SetVOI 0 360 0 239 $i $i
    $pvExtractGrid UpdateParameterWidgets
    $pvExtractGrid AcceptCallback
}


# Turn off the cube-axis actor.
$pvExtractGridOutput SetCubeAxesVisibility 0



# ------------------  Rotate the globe showing temperature -------------

# No need for these to slow us down.
$pvCont SetVisibility 0
$pvFloor SetVisibility 0


[$pvExtractGrid GetVTKSource] SetVOI 0 360 0 239 1 1
$pvExtractGrid UpdateParameterWidgets
$pvExtractGrid AcceptCallback


#Rotate the camera
set cam [Ren1 GetActiveCamera]
$cam SetFocalPoint 0 0 0

for {set i 0} {$i < 360} { set i [expr $i + $RotationStep]} {
    puts "Rotate $i"
    $cam Azimuth $RotationStep
    RenWin1 Render
}

# --------------- Iterate through Salinity IsoSurfaces.----------

$pvFloor SetVisibility 1
$pvCont SetVisibility 1
$pvContour SetVisibility 1
$pvExtractGrid SetVisibility 0

$pvWindow SetCurrentPVSource $pvContour
update

for {set i 0.035} {$i <= 0.039} { set i [expr $i + $IsoStep]} {
    puts "Contour value $i"
    [$pvContour GetVTKSource] SetValue 0 $i
    $pvExtractGrid UpdateParameterWidgets
    $pvExtractGrid AcceptCallback
}

$pvContour SetVisibility 0






