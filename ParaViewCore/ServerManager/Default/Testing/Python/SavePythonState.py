# test driver for the second half of the SavePythonState test. it
# sets up the testing environment, loads the state file (passed
# on the command line), and verifys the resulting image

import sys
from paraview import smtesting

smtesting.ProcessCommandLineArguments()

execfile(sys.argv[1])

RenderView1.ViewSize = [300, 300]
RenderView1.SMProxy.UpdateVTKObjects()

if not smtesting.DoRegressionTesting(RenderView1.SMProxy):
  # This will lead to VTK object leaks.
  sys.exit(1)
