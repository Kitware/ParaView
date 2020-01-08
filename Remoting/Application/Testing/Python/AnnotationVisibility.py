# Test BUG #0015025: Toggling annotation text visibility in Catalyst/batch doesn't work.

from paraview.simple import *
from paraview import smtesting
smtesting.ProcessCommandLineArguments()

v = CreateRenderView()
v.OrientationAxesVisibility = 0

HeadingText = Text(Text="Hello World")
HeadingRep = Show()
HeadingRep.WindowLocation = 'UpperCenter'
HeadingRep.FontSize = 18
HeadingRep.TextScaleMode = 'Viewport'

Show()
Render()
# raw_input("Visible: %d: " % HeadingRep.Visibility)

Hide()
Render()
#raw_input("Visible: %d: " % HeadingRep.Visibility)

Show()
Render()
#raw_input("Visible: %d: " % HeadingRep.Visibility)

Hide()
Render()
#raw_input("Visible: %d: " % HeadingRep.Visibility)

Show()
Render()
#raw_input("Visible: %d: " % HeadingRep.Visibility)

if not smtesting.DoRegressionTesting(v.SMProxy):
  # This will lead to VTK object leaks.
  import sys
  sys.exit(1)
