# This test checks the formatting of the trace output.
#
#  - if a single property is set, direct assignment proxy.propName = 'foo'
#    should be used.
#  - if more than one property is set at once, the proxy.Set(propName='foo',)
#    method should be used.
#

from paraview.simple import *
from paraview import smtrace
from paraview import smtesting

settings = servermanager.vtkPVGeneralSettings()
settings.SetScalarBarMode(settings.MANUAL_SCALAR_BARS)

# Process command line args and get temp dir
smtesting.ProcessCommandLineArguments()
tempDir = smtesting.TempDir

# Start trace
config = smtrace.start_trace()

def create_pipeline():
    Sphere(
        Radius=10,
    )

create_pipeline()

# Stop trace and grab the trace output string
trace_string = smtrace.stop_trace()

assert "sphere1.Radius = 10.0" in trace_string

# Start trace
config = smtrace.start_trace()

def create_pipeline():
    Sphere(
        Radius=10,
        Center=[1.0, 2.0 ,3.0],
    )

create_pipeline()

# Stop trace and grab the trace output string
trace_string = smtrace.stop_trace()

assert """
sphere2.Set(
    Center=[1.0, 2.0, 3.0],
    Radius=10.0,
)
""" in trace_string
