catch {load vtktcl}
load vtkKWWidgetsTcl
load vtkKWParaViewTcl

if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl


vtkInteractorStyle defaultStyle
vtkInteractorStyleGridExtent extentStyle



# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# cut data
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update


#extract plane
vtkStructuredGridGeometryFilter compPlane
    compPlane SetInput [pl3d GetOutput]
    compPlane SetExtent 0 100 0 100 9 9
vtkPolyDataMapper planeMapper
    planeMapper SetInput [compPlane GetOutput]
    planeMapper ScalarVisibilityOff
vtkActor planeActor
    planeActor SetMapper planeMapper
    [planeActor GetProperty] SetRepresentationToWireframe
    [planeActor GetProperty] SetColor 0 0 0

#outline
vtkStructuredGridOutlineFilter outline
    outline SetInput [pl3d GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
set outlineProp [outlineActor GetProperty]
eval $outlineProp SetColor 0 0 0



# sub extent manipulated by the style
vtkExtractGrid subgrid
    subgrid SetInput [pl3d GetOutput]
    subgrid SetVOI 2 50 2 20 2 20
vtkStructuredGridOutlineFilter outline2
    outline2 SetInput [subgrid GetOutput]
vtkPolyDataMapper outlineMapper2
    outlineMapper2 SetInput [outline2 GetOutput]
vtkActor outlineActor2
    outlineActor2 SetMapper outlineMapper2
    [outlineActor2 GetProperty] SetColor 0 0 0


# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor outlineActor2
ren1 AddActor planeActor
ren1 SetBackground 1 1 1
renWin SetSize 500 500


extentStyle SetStructuredGrid [pl3d GetOutput]
extentStyle SetExtent 2 50 2 20 2 20
extentStyle SetCallbackMethod MyCallback

proc MyCallback {} {
  set type [extentStyle GetCallbackType]
  eval subgrid SetVOI [extentStyle GetExtent]
  extentStyle DefaultCallback $type
}

iren SetInteractorStyle extentStyle

proc ToggleStyle {} {
    if {[string equal [iren GetInteractorStyle] defaultStyle]} {
	iren SetInteractorStyle planeStyle
    } else {
	iren SetInteractorStyle defaultStyle
    }
}



set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 3.95297 50
$cam1 SetFocalPoint 9.71821 0.458166 29.3999
$cam1 SetPosition 2.7439 -37.3196 38.7167
$cam1 ComputeViewPlaneNormal
$cam1 SetViewUp -0.16123 0.264271 0.950876
iren Initialize

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

#renWin SetFileName "probe.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .











