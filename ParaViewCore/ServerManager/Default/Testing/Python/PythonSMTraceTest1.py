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

# Create a render view before starting trace
ren = CreateRenderView()

# Start trace
config = smtrace.start_trace()

########################################################
# Begin build pipeline
def create_pipeline():
  w = Wavelet()
  Show()
  Render()

  # note property changes won't be recorded directly so don't do any outside the
  # constructor and expect that to work.
  contour = Contour(ComputeScalars=1,
      Isosurfaces=[100, 150, 200])
  Show()
  #Hide(w)
  Render()
  ColorBy(value=("POINTS", "RTData"))
  GetDisplayProperties().SetScalarBarVisibility(ren, True)
  Render()

  clip = Clip()
  Show()
  Hide(contour)
  Render()

create_pipeline()

# End build pipeline
########################################################

# Stop trace and grab the trace output string
trace_string = smtrace.stop_trace()
print(trace_string)
# Uncomment these lines to print the trace string or save it to a file
print(trace_string)
#smtrace.save_trace(tempDir + "/PythonSMTraceTest1.py")

# Clear all the sources
for source in GetSources().values():
    Delete(source)


# Confirm that all the representations have been removed from the view except 1
# for the scalar bar.
if len(ren.Representations) != 1:
    print("View should not have any representations except the scalar bar!")
    sys.exit(1)

# destroy the scalar bar too
Delete(ren.Representations[0])

if len(ren.Representations) != 0:
    print("View should not have any representations at this point!")
    sys.exit(1)

# Confirm that the clip filter has been removed
if GetSources():
    print("All sources should have be removed.")
    sys.exit(1)

# change camera to confirm that trace restores it.
ren.CameraPosition = [-1, -1, -1]
Render()

# Compile the trace code and run it
code = compile(trace_string, "<string>", "exec")
exec(code)

# Confirm that the clip filter has been recreated
clip = [x for x in GetSources().values() if x.GetXMLLabel() == "Clip"]
if not clip:
    print("After replaying trace, could not find Clip filter.")
    sys.exit(1)

clip[0].Invert = 0

# Do a screenshot regression test
if not smtesting.DoRegressionTesting(ren.SMProxy):
    raise smtesting.TestError('Image comparison failed.')
