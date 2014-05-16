# This test runs through the steps described in the python trace tutorial
# to appear in a Kitware Source newsletter.
#
#  Trace is enabled and the following actions are captured and then replayed:
#
#  An octree fractal source is created, dimension set to 3, contour filter is
#  applied, then the active source is changed back to the fractal source and
#  an extract cells by region filter is applied.  The clip plane of the extract
#  filter is adjusted, the camera is adjusted, a colormap is applied, and a 
#  scalar bar widget is added to the scene and its position is adjusted.
#

from paraview.simple import *
from paraview import smtrace
from paraview import smtesting
import sys

settings = servermanager.vtkPVGeneralSettings()
settings.SetScalarBarMode(settings.MANUAL_SCALAR_BARS)

# Process command line args and get temp dir
smtesting.ProcessCommandLineArguments()
tempDir = smtesting.TempDir

# Set APPLY to be an alias for append_trace function.  When running paraview
# from the gui append_trace is called whenever the user hits the Apply button.
APPLY = smtrace.append_trace

# Create a render view before starting trace
ren = CreateRenderView()

# Start trace
smtrace.start_trace(CaptureAllProperties=True, UseGuiName=True)

########################################################
# Begin build pipeline
wavelet = Wavelet(guiName="My Wavelet")

APPLY()

ren = GetRenderView()
waveletRep = Show()

contour = Contour(guiName="My Contour")
contour.PointMergeMethod = "Uniform Binning"
contour.ContourBy = ['POINTS', 'RTData']
contour.Isosurfaces = [157.09096527099609]

APPLY()

contourRep = Show()
waveletRep.Visibility = 0

SetActiveSource(wavelet)
clip = Clip(guiName="My Clip", ClipType="Plane" )
clip.Scalars = ['POINTS', 'RTData']
clip.ClipType = "Plane"

APPLY()

clipRep = Show()

a1_RTData_PVLookupTable = GetLookupTableForArray( "RTData", 1, NanColor=[0.5, 0.5, 0.5], RGBPoints=[37.4, 1.0, 0.0, 0.0, 276.8, 0.0, 0.0, 1.0], VectorMode='Magnitude', ColorSpace='HSV', ScalarRangeInitialized=1.0 )

a1_RTData_PiecewiseFunction = CreatePiecewiseFunction()

clipRep.ScalarOpacityFunction = a1_RTData_PiecewiseFunction
clipRep.ColorArrayName = 'RTData'
clipRep.ScalarOpacityUnitDistance = 1.8307836667054274
clipRep.LookupTable = a1_RTData_PVLookupTable

APPLY()

contour.ComputeScalars = 1

APPLY()

contourRep.ColorArrayName = 'RTData'
contourRep.LookupTable = a1_RTData_PVLookupTable

clip.ClipType.Normal = [0.0, 0.0, 1.0]

APPLY()

bar = CreateScalarBar( Orientation='Horizontal', Title='RTData', ComponentTitle="", Position2=[0.6, 0.2], Enabled=1, LabelFontSize=12, LookupTable=a1_RTData_PVLookupTable, TitleFontSize=20, Position=[0.2, 0.0] )
GetRenderView().Representations.append(bar)

APPLY()

ren.CameraFocalPoint = [1.1645689199943594, -3.5914371885980554, 0.54379903964477228]
ren.CameraClippingRange = [31.266692271900439, 107.69973132887634]
ren.CameraViewUp = [-0.14069051476636268, 0.84334441613242261, 0.51862932315193999]
ren.CameraPosition = [26.064234130736576, 31.908865377615712, -50.428704912804747]

APPLY()

# End build pipeline
########################################################

# Stop trace and grab the trace output string
smtrace.stop_trace()
trace_string = smtrace.get_trace_string()
# Uncomment these lines to print the trace string or save it to a file
#print trace_string
#smtrace.save_trace(tempDir + "/PythonSMTraceTest1.py")

# Clear all the sources
Delete(clip)
Delete(contour)
Delete(wavelet)
Delete(bar)

# Confirm that all the representations have been removed from the view
if len(ren.Representations):
    print "View should not have any representations."
    sys.exit(1)

# Confirm that the clip filter has been removed
if FindSource("My Clip"):
    print "Clip filter should have been cleaned up."
    sys.exit(1)

# Compile the trace code and run it
code = compile(trace_string, "<string>", "exec")
exec(code)

# Confirm that the clip filter has been recreated
if not FindSource("My Clip"):
    print "After replaying trace, could not find Clip filter."
    sys.exit(1)

# Do a screenshot regression test
if not smtesting.DoRegressionTesting(ren.SMProxy):
    raise smtesting.TestError('Image comparison failed.')

