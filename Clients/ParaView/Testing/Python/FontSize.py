# state file generated using paraview version 5.8.0

from paraview.simple import *
#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

# Create a new 'Render View'
renderView1 = CreateView('RenderView')
renderView1.ViewSize = [1280, 800]
renderView1.AxesGrid = 'GridAxes3DActor'
renderView1.OrientationAxesVisibility = 0
renderView1.StereoType = 'Crystal Eyes'
renderView1.CameraPosition = [0.0, 0.0, 6.6921304299024635]
renderView1.CameraFocalDisk = 1.0
renderView1.CameraParallelScale = 1.7320508075688772

SetActiveView(None)

# create new layout object 'Layout #1'
layout1 = CreateLayout(name='Layout #1')
layout1.AssignView(0, renderView1)

SetActiveView(renderView1)

text1 = Text()
text1.Text = 'Interstellar clouds of gas and dust collapse to form stars'

text1Display = Show(text1, renderView1, 'TextSourceRepresentation')
text1Display.FontFamily = 'Times'
text1Display.FontSize = 10
text1Display.WindowLocation = 'AnyLocation'
text1Display.Position = [0.1, 0.0]

text2 = Text()
text2.Text = text1.Text

text2Display = Show(text2, renderView1, 'TextSourceRepresentation')
text2Display.FontFamily = 'Times'
text2Display.FontSize = 11
text2Display.WindowLocation = 'AnyLocation'
text2Display.Position = [0.1, 0.1]

text3 = Text()
text3.Text = text1.Text

text3Display = Show(text3, renderView1, 'TextSourceRepresentation')
text3Display.FontFamily = 'Times'
text3Display.FontSize = 12
text3Display.WindowLocation = 'AnyLocation'
text3Display.Position = [0.1, 0.2]

text4 = Text()
text4.Text = text1.Text

text4Display = Show(text4, renderView1, 'TextSourceRepresentation')
text4Display.FontFamily = 'Times'
text4Display.FontSize = 13
text4Display.WindowLocation = 'AnyLocation'
text4Display.Position = [0.1, 0.3]

text5 = Text()
text5.Text = text1.Text

text5Display = Show(text5, renderView1, 'TextSourceRepresentation')
text5Display.FontFamily = 'Times'
text5Display.FontSize = 14
text5Display.WindowLocation = 'AnyLocation'
text5Display.Position = [0.1, 0.4]
text5Display.TextScaleMode = 'Viewport'

from os.path import join
from paraview.vtk.vtkTestingRendering import vtkTesting
import sys
testing = vtkTesting()
for arg in sys.argv:
    testing.AddArgument(arg)
for x in range(1, len(sys.argv)):
    if sys.argv[x] == "-B" and len(sys.argv) > (x+1):
        testing.AddArgument("-V")
        testing.AddArgument(join(sys.argv[x+1], "FontSize.png"))

testimage = join(testing.GetTempDirectory(), "FontSize.png")
SaveScreenshot(testimage, layout=layout1, ImageResolution=[1280,800])

result = testing.RegressionTest(testimage, 10)
if result == testing.DO_INTERACTOR:
    sys.exit(0)
elif result == testing.NOT_RUN:
    sys.exit(125)
elif result == testing.FAILED:
    raise RuntimeError("test failed!")
