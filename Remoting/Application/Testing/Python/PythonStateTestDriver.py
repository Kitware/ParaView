# Test driver for the second half of the PythonStateTestDriver test. It
# sets up the testing environment, loads the state file (passed
# on the command line), and verifies the resulting image.

import sys
from paraview import smtesting
print(sys.argv)
smtesting.ProcessCommandLineArguments()

exec(open(sys.argv[1]).read())


_view = GetActiveView()
_view.ViewSize = [300, 300]
_view.StillRender()

if not smtesting.DoRegressionTesting(_view.SMProxy):
  # This will lead to VTK object leaks.
  sys.exit(1)
