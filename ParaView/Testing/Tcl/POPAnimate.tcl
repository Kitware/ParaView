
set RotationStep 5
set DepthStep 9

# Setup color map to show temperature.
set pvExtractGridOutput [$pvExtractGrid GetNthPVOutput 0]
[$pvExtractGridOutput GetColorMenu] SetValue {Point Temperature}
$pvExtractGridOutput ColorByPointFieldComponent Temperature 0
[$pvExtractGridOutput GetColorMapMenu] SetValue {Blue to Red}
$pvExtractGridOutput ChangeColorMap

# Turn on scalar bar.
$pvExtractGridOutput SetScalarBarVisibility 1
$pvExtractGridOutput SetScalarBarOrientationToHorizontal

# Turn on the cube axes.
$pvExtractGridOutput SetCubeAxesVisibility 0

# set the continent color to match the Temperature of 0.
$pvContOutput ChangeActorColor 0 [expr 197.0/255.0] 1.0

# Loop over all of the depth values.
for {set i 1} {$i < 30} {set i [expr $i + $DepthStep]} {
    [$pvExtractGrid GetVTKSource] SetVOI 0 360 0 239 $i $i
    $pvExtractGrid UpdateParameterWidgets
    $pvExtractGrid AcceptCallback
}



# Setup color map to show Salinity.
[$pvExtractGridOutput GetColorMenu] SetValue {Point Salinity}
$pvExtractGridOutput ColorByPointFieldComponent Salinity 0
[$pvExtractGridOutput GetColorMapMenu] SetValue {Red to Blue}
$pvExtractGridOutput ChangeColorMap
$pvExtractGridOutput SetScalarRange 0.03 0.0403

# Change the color of the continents to match the scalar range.
$pvContOutput ChangeActorColor 1 0 0


# Loop over all of the depth values.
for {set i 1} {$i < 30} {set i [expr $i + $DepthStep]} {
    [$pvExtractGrid GetVTKSource] SetVOI 0 360 0 239 $i $i
    $pvExtractGrid UpdateParameterWidgets
    $pvExtractGrid AcceptCallback
}

[$pvExtractGrid GetVTKSource] SetVOI 0 360 0 239 5 5
$pvExtractGrid UpdateParameterWidgets
$pvExtractGrid AcceptCallback

# Turn on the cube-axis actor.
$pvExtractGridOutput SetCubeAxesVisibility 0


#Rotate the camera
set cam [Ren1 GetActiveCamera]
$cam SetFocalPoint 0 0 0

for {set i 0} {$i < 540} { set i [expr $i + $RotationStep]} {
    $cam Azimuth $RotationStep
    $pvView Render
}

$pvView EventuallyRender

for {set i 0} {$i < 540} { set i [expr $i + $RotationStep]} {
    $cam Azimuth $RotationStep
    $pvView Render
}

$pvView EventuallyRender




# Iterate through Salinity IsoSurfaces.









