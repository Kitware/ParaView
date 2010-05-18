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
import SMPythonTesting
import sys

# Process command line args and get temp dir
SMPythonTesting.ProcessCommandLineArguments()
tempDir = SMPythonTesting.TempDir

# Set APPLY to be an alias for append_trace function.  When running paraview
# from the gui append_trace is called whenever the user hits the Apply button.
APPLY = smtrace.append_trace

# Create a render view before starting trace
ren = CreateRenderView()

# Start trace
smtrace.start_trace(CaptureAllProperties=True, UseGuiName=True)

########################################################
# Begin build pipeline
fractal = OctreeFractal(guiName="Octree Fractal")
APPLY()

fractalRep = Show()

lut = CreateLookupTable(RGBPoints=[2.2222592830657959, 0.23000000000000001, 0.29899999999999999, 0.754, 100.0, 0.70599999999999996, 0.016, 0.14999999999999999], VectorMode='Magnitude', ColorSpace='Diverging')
fractalRep.ColorArrayName = 'FractalIterations'
fractalRep.LookupTable = lut

fractal.Dimension = 3
lut.RGBPoints = [1.1045577526092529, 0.23000000000000001, 0.29899999999999999, 0.754, 100.0, 0.70599999999999996, 0.016, 0.14999999999999999]

APPLY()

contour = Contour(guiName="Contour")
contour.ContourBy = ['POINTS', 'FractalIterations']
contour.Isosurfaces = [50.552278876304626]

APPLY()

contourRep = Show()
fractalRep.Visibility = 0

SetActiveSource(fractal)
extract = ExtractCellsByRegion( IntersectWith="Plane", guiName="Extract Cells By Region" )
extract.IntersectWith.Origin = [-0.5, 0.0, 1.0]
extract.IntersectWith = "Plane"

APPLY()

extractRep = Show()

extractRep.ColorArrayName = 'FractalIterations'
extractRep.ColorAttributeType = 'POINT_DATA'
extractRep.LookupTable = lut

ren.CameraViewUp = [0.020491480527326099, -0.99935907640320587, -0.029351927301794135]
ren.CameraPosition = [-4.1551704083097531, 0.051380834701542075, -4.3011743474296438]
ren.CameraClippingRange = [1.5266102810152136, 11.791585857626885]
ren.CameraFocalPoint = [-0.49999999999999978, 1.4589157341685681e-17, -1.1307471589527126e-16]

extract.IntersectWith.Origin = [-0.52722651942313892, 0.034258501654010676, 0.41162920375900314]
extract.IntersectWith.Normal = [-0.046146966413950434, 0.058065663871685436, -0.99724562479357626]

APPLY()

bar = CreateScalarBar()
bar.LookupTable = lut
ren.Representations.append(bar)
bar.Title='FractalIterations'
bar.Position2=[0.18306122448979578, 0.68654434250764518]
bar.Position=[0.80673469387755092, 0.20718654434250777]
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
Delete(extract)
Delete(contour)
Delete(fractal)
Delete(bar)

# Confirm that all the representations have been removed from the view
if len(ren.Representations):
    print "View should not have any representations."
    sys.exit(1)

# Confirm that the extract cells filter has been removed
if FindSource("Extract Cells By Region"):
    print "Extract Cells filter should have been cleaned up."
    sys.exit(1)

# Compile the trace code and run it
code = compile(trace_string, "<string>", "exec")
exec(code)

# Confirm that the extract cells filter has been recreated
if not FindSource("Extract Cells By Region"):
    print "After replaying trace, could not find Extract Cells filter."
    sys.exit(1)

# Do a screenshot regression test
if not SMPythonTesting.DoRegressionTesting(ren.SMProxy):
    raise SMPythonTesting.Error('Image comparison failed.')

