catch {load vtktcl}
load vtkKWWidgetsTcl
load vtkKWParaViewTcl

if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


# get the interactor ui
source $VTK_TCL/vtkInt.tcl



vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff
reader UpdateInformation


set XMIN 20
set XMAX 235
set YMIN 20
set YMAX 235
set ZMIN 20
set ZMAX 72


vtkOutlineSource whole
	eval whole SetBounds [[reader GetOutput] GetWholeExtent]
vtkPolyDataMapper wholeMapper
	wholeMapper SetInput [whole GetOutput]
vtkActor wholeActor
	wholeActor SetMapper wholeMapper


vtkOutlineSource extentOutline
	extentOutline SetBounds $XMIN $XMAX $YMIN $YMAX $ZMIN $ZMAX
vtkPolyDataMapper extentMapper
	extentMapper SetInput [extentOutline GetOutput]
vtkActor extentActor
	extentActor SetMapper extentMapper


vtkInteractorStyleImageExtent extentStyle
	extentStyle SetExtent $XMIN $XMAX $YMIN $YMAX $ZMIN $ZMAX
	extentStyle SetImageData [reader GetOutput]

extentStyle SetCallbackMethod MyCallback

proc MyCallback {} {
  set type [extentStyle GetCallbackType]
  # Ignore origin (0,0,0) and spacing(1,1,1).
  eval extentOutline SetBounds [extentStyle GetExtent]
  extentStyle DefaultCallback $type
}







# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin



# Add the actors to the renderer, set the background and size
ren1 AddActor wholeActor
ren1 AddActor extentActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 500 500


set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 3.95297 50
$cam1 SetFocalPoint 127.5 127.5 46
$cam1 SetPosition 593.6 593.6 550.0
$cam1 ComputeViewPlaneNormal


# render the image
iren SetUserMethod {wm deiconify .vtkInteract}
renWin Render



iren SetInteractorStyle extentStyle



# prevent the tk window from showing up then start the event loop
wm withdraw .





