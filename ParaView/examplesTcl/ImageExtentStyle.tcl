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

set XMIN 0
set XMAX 255
set YMIN 0
set YMAX 255
set ZMIN 0
set ZMAX 92

vtkOutlineSource whole
#whole SetInput [reader GetOutput]
whole SetBounds $XMIN $XMAX $YMIN $YMAX $ZMIN $ZMAX


vtkPolyDataMapper wholeMapper
wholeMapper SetInput [whole GetOutput]

vtkActor wholeActor
wholeActor SetMapper wholeMapper





# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create a plane source and actor
vtkPlaneSource plane
vtkPolyDataMapper  planeMapper
planeMapper SetInput [plane GetOutput]
vtkActor planeActor
planeActor SetMapper planeMapper


# load in the texture map
#
vtkTexture atext
vtkPNMReader pnmReader
pnmReader SetFileName "$VTK_DATA/masonry.ppm"
atext SetInput [pnmReader GetOutput]
atext InterpolateOn
planeActor SetTexture atext


# Add the actors to the renderer, set the background and size
ren1 AddActor wholeActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 500 500

# render the image
iren SetUserMethod {wm deiconify .vtkInteract}
renWin Render



vtkInteractorExtentStyle extentStyle
extentStyle SetBounds $XMIN $XMAX $YMIN $YMAX $ZMIN $ZMAX
iren SetInteractorStyle extentStyle



# prevent the tk window from showing up then start the event loop
wm withdraw .





