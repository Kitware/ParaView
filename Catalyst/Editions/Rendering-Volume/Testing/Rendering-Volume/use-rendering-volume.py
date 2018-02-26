#### import the simple module from the paraview
from paraview.simple import *
#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

# create a new 'Wavelet'
#myidpvti = Wavelet()
import vtk
id = vtk.vtkImageData()

pm = paraview.servermanager.vtkProcessModule.GetProcessModule()
globalController = pm.GetGlobalController()
procid = globalController.GetLocalProcessId()
nump = globalController.GetNumberOfProcesses()

e = [1, 1, 1, 1, 1, 1]
we = [-10, 10, -10, 10, -10, 10]
et = vtk.vtkExtentTranslator()
et.PieceToExtentThreadSafe(procid, nump, 0, we, e, 3, 0)

id.SetExtent(e)
aa = vtk.vtkFloatArray()
aa.SetName("RTData")
aa.SetNumberOfTuples(id.GetNumberOfPoints())
for v in range(id.GetNumberOfPoints()):
  aa.SetValue(v, v)

id.GetPointData().AddArray(aa)

myidpvti = PVTrivialProducer()
myidpvti.GetClientSideObject().SetOutput(id)
myidpvti.WholeExtent = we

writer = XMLPImageDataWriter(FileName="myid.pvti")
writer.UpdatePipeline()

renderView1 = GetActiveViewOrCreate('RenderView')
# uncomment following to set a specific view size
# renderView1.ViewSize = [963, 1145]

# get display properties
myidpvtiDisplay = GetDisplayProperties(myidpvti, view=renderView1)

# change representation type
myidpvtiDisplay.SetRepresentationType('Surface')

# set scalar coloring
ColorBy(myidpvtiDisplay, ('POINTS', 'RTData'))

# rescale color and/or opacity maps used to include current data range
myidpvtiDisplay.RescaleTransferFunctionToDataRange(True, False)

# show color bar/color legend
#myidpvtiDisplay.SetScalarBarVisibility(renderView1, True) # ACB add this...

# get color transfer function/color map for 'RTData'
rTDataLUT = GetColorTransferFunction('RTData')
rTDataLUT.RGBPoints = [37.35310363769531, 0.231373, 0.298039, 0.752941, 157.0909652709961, 0.865003, 0.865003, 0.865003, 276.8288269042969, 0.705882, 0.0156863, 0.14902]
rTDataLUT.ScalarRangeInitialized = 1.0

# current camera placement for renderView1
renderView1.CameraPosition = [0.0, 0.0, 78.78518764863527]
renderView1.CameraParallelScale = 20.59395820006609

# save screenshot
SaveScreenshot('idsurface.png', renderView1, ImageResolution=[963, 1145],
    OverrideColorPalette='PrintBackground',
    TransparentBackground=1)

print("created imagedata surface rendered screenshot")

renderView1 = GetActiveViewOrCreate('RenderView')
# uncomment following to set a specific view size
# renderView1.ViewSize = [963, 1145]

# get display properties
myidpvtiDisplay = GetDisplayProperties(myidpvti, view=renderView1)

# set scalar coloring
ColorBy(myidpvtiDisplay, ('POINTS', 'RTData'))

# rescale color and/or opacity maps used to include current data range
myidpvtiDisplay.RescaleTransferFunctionToDataRange(True, True)

# change representation type

print  "AAAAAAAAAAAAAAAAAAAAAAAAAAa ", myidpvtiDisplay.Representation

myidpvtiDisplay.SetRepresentationType('Volume')
print "POST_______________________ ", myidpvtiDisplay
#for i in myidpvtiDisplay:
#  print i.GetClassName(), i, dir(i)

print  "BBBBBBBBBBBBB ", myidpvtiDisplay.Representation

myidpvtiDisplay.Representation = 'Volume'
print  "CCCCCCCC ", myidpvtiDisplay.Representation

# get color transfer function/color map for 'RTData'
rTDataLUT = GetColorTransferFunction('RTData')
rTDataLUT.RGBPoints = [0.0, 0.231373, 0.298039, 0.752941, 4630.0, 0.865003, 0.865003, 0.865003, 9260.0, 0.705882, 0.0156863, 0.14902]
rTDataLUT.ScalarRangeInitialized = 1.0

# reset view to fit data
renderView1.ResetCamera()

#### saving camera placements for all active views

# current camera placement for renderView1
renderView1.CameraPosition = [0.0, -78.78518764863527, 0.0]
renderView1.CameraFocalPoint = [0.0, 1e-20, 0.0]
renderView1.CameraViewUp = [0.0, 0.0, 1.0]
renderView1.CameraParallelScale = 20.59395820006609


# save screenshot
SaveScreenshot('idvolume.png', renderView1, ImageResolution=[963, 1145],
#    OverrideColorPalette='PrintBackground',
    TransparentBackground=1)

print("created imagedata volume rendered screenshot")

# create a new 'Clip'
clip1 = Clip(Input=myidpvti)
clip1.ClipType = 'Plane'
clip1.Scalars = ['POINTS', 'RTData']
clip1.Value = 157.0909652709961


# show data in view
clip1Display = Show(clip1, renderView1)

# get opacity transfer function/opacity map for 'RTData'
rTDataPWF = GetOpacityTransferFunction('RTData')
rTDataPWF.Points = [0.0, 0.0, 0.5, 0.0, 9260.0, 1.0, 0.5, 0.0]
rTDataPWF.ScalarRangeInitialized = 1

# trace defaults for the display properties.
clip1Display.Representation = 'Surface'
clip1Display.ColorArrayName = ['POINTS', 'RTData']
clip1Display.LookupTable = rTDataLUT
clip1Display.EdgeColor = [0.0, 0.0, 0.0]
clip1Display.OSPRayScaleArray = 'RTData'
clip1Display.OSPRayScaleFunction = 'PiecewiseFunction'
clip1Display.SelectOrientationVectors = 'None'
clip1Display.ScaleFactor = 2.0
clip1Display.SelectScaleArray = 'None'
clip1Display.GlyphType = 'Arrow'
clip1Display.GlyphTableIndexArray = 'None'
clip1Display.DataAxesGrid = 'GridAxesRepresentation'
clip1Display.SelectionCellLabelFontFile = ''
clip1Display.SelectionPointLabelFontFile = ''
clip1Display.PolarAxes = 'PolarAxesRepresentation'
clip1Display.ScalarOpacityFunction = rTDataPWF
clip1Display.ScalarOpacityUnitDistance = 1.7320508075688779
clip1Display.GaussianRadius = 1.0
clip1Display.SetScaleArray = ['POINTS', 'RTData']
clip1Display.ScaleTransferFunction = 'PiecewiseFunction'
clip1Display.OpacityArray = ['POINTS', 'RTData']
clip1Display.OpacityTransferFunction = 'PiecewiseFunction'

# init the 'GridAxesRepresentation' selected for 'DataAxesGrid'
clip1Display.DataAxesGrid.XTitleFontFile = ''
clip1Display.DataAxesGrid.YTitleFontFile = ''
clip1Display.DataAxesGrid.ZTitleFontFile = ''
clip1Display.DataAxesGrid.XLabelFontFile = ''
clip1Display.DataAxesGrid.YLabelFontFile = ''
clip1Display.DataAxesGrid.ZLabelFontFile = ''

# init the 'PolarAxesRepresentation' selected for 'PolarAxes'
clip1Display.PolarAxes.PolarAxisTitleFontFile = ''
clip1Display.PolarAxes.PolarAxisLabelFontFile = ''
clip1Display.PolarAxes.LastRadialAxisTextFontFile = ''
clip1Display.PolarAxes.SecondaryRadialAxesTextFontFile = ''

# init the 'PiecewiseFunction' selected for 'ScaleTransferFunction'
clip1Display.ScaleTransferFunction.Points = [0.0, 0.0, 0.5, 0.0, 9260.0, 1.0, 0.5, 0.0]

# init the 'PiecewiseFunction' selected for 'OpacityTransferFunction'
clip1Display.OpacityTransferFunction.Points = [0.0, 0.0, 0.5, 0.0, 9260.0, 1.0, 0.5, 0.0]

# show color bar/color legend
#clip1Display.SetScalarBarVisibility(renderView1, True) # ACB add this...

# update the view to ensure updated data information
renderView1.Update()

# change representation type
clip1Display.SetRepresentationType('Volume')

# hide data in view
Hide(myidpvti, renderView1)

# current camera placement for renderView1
renderView1.CameraPosition = [0.0, 0.0, 78.78518764863527]
renderView1.CameraParallelScale = 20.59395820006609
renderView1.CameraViewUp = [1.0, 0.0, .0]

# save screenshot
SaveScreenshot('ugvolume.png', renderView1, ImageResolution=[963, 1145]) #, OverrideColorPalette='PrintBackground', TransparentBackground=1)

#### saving camera placements for all active views

# current camera placement for renderView1
renderView1.CameraPosition = [0.0, 0.0, 78.78518764863527]
renderView1.CameraParallelScale = 20.59395820006609

#### uncomment the following to render all views
# RenderAllViews()
# alternatively, if you want to write images, you can use SaveScreenshot(...).
