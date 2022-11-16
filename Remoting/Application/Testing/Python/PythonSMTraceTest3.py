# This test runs through the steps described in the python trace tutorial
# to appear in a Kitware Source newsletter.
#
#  Trace is enabled and the following actions are captured and then replayed:
#
#  A sphere source is created then a PythonAnnotation filter is created.
#  Its expression contains a '\n'.
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
  w = Sphere()
  Show()
  Render()

  # note property changes won't be recorded directly so don't do any outside the
  # constructor and expect that to work.
  python_annotation = PythonAnnotation(registrationName='PythonAnnotation1', Input=w, Expression='"Foo\\nBar"')
  Show()
  #Hide(w)
  Render()

create_pipeline()

# End build pipeline
########################################################

# Stop trace and grab the trace output string
trace_string = smtrace.stop_trace()
print(trace_string)

# The multiline expression in the PythonAnnotation should not be processed
# but taken 'as is'
assert r"""pythonAnnotation1.Expression = '"Foo\\nBar"'""" in trace_string

# Uncomment these lines to print the trace string or save it to a file
#smtrace.save_trace(tempDir + "/PythonSMTraceTest1.py")

# Clear all the sources
for source in GetSources().values():
    Delete(source)

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

# Do a screenshot regression test
if not smtesting.DoRegressionTesting(ren.SMProxy):
    raise smtesting.TestError('Image comparison failed.')
