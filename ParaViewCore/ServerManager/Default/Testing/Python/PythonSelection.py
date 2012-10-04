from paraview.simple import *

import SMPythonTesting
SMPythonTesting.ProcessCommandLineArguments()

filename = SMPythonTesting.DataDir + '/Data/can.ex2'
can_ex2 = OpenDataFile(filename)
Show()
SelectCells("id > 1000")

RenderView1 = Render()
RenderView1.CameraViewUp = [0.0, 0.0, 1.0]
RenderView1.CameraPosition = [0.21706008911132812, 55.740573746856327, -5.1109471321105957]
RenderView1.CameraFocalPoint = [0.21706008911132812, 4.0, -5.1109471321105957]
RenderView1 = Render()

if not SMPythonTesting.DoRegressionTesting(RenderView1.SMProxy):
  # This will lead to VTK object leaks.
  import sys
  sys.exit(1)
