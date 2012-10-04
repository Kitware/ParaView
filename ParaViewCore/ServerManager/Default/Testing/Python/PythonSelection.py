from paraview.simple import *

import SMPythonTesting
SMPythonTesting.ProcessCommandLineArguments()

s = Sphere()
c =Cone(Resolution=10)
GroupDatasets(Input=[s,c])
Show()

SelectCells("id > 2")


RenderView1 = Render()

if not SMPythonTesting.DoRegressionTesting(RenderView1.SMProxy):
  # This will lead to VTK object leaks.
  import sys
  sys.exit(1)
