from paraview.simple import *
from paraview.vtk.util.misc import vtkGetTempDir
from paraview.vtk.vtkTestingRendering import vtkTesting
from os.path import join
import sys

# create a new 'Wavelet'
wavelet1 = Wavelet(registrationName='Wavelet1')

# get active view
renderView1 = GetActiveViewOrCreate('RenderView')

# show data in view
wavelet1Display = Show(wavelet1, renderView1, 'UniformGridRepresentation')

# set scalar coloring
ColorBy(wavelet1Display, ('POINTS', 'RTData'))

# change representation type
wavelet1Display.SetRepresentationType('Surface')

# hide color bar/color legend
wavelet1Display.SetScalarBarVisibility(renderView1, False)

# get layout
layout1 = GetLayout()

# layout/tab size in pixels
layout1.SetSize(300, 300)

# current camera placement for renderView1
renderView1.CameraPosition = [30.273897726939246, 40.8733980301544, 43.48927935675712]
renderView1.CameraViewUp = [-0.3634544237682163, 0.7916848767068606, -0.49105594165731975]
renderView1.CameraParallelScale = 17.320508075688775

# save screenshot
testimage = join(vtkGetTempDir(), "SaveTransparentImages-Test.png")
SaveScreenshot(testimage, renderView1, ImageResolution=[300, 300],
    TransparentBackground=1)

testing = vtkTesting()
for arg in sys.argv:
    testing.AddArgument(arg)
for x in range(1, len(sys.argv)):
    if sys.argv[x] == "-B" and len(sys.argv) > (x+1):
        testing.AddArgument("-V")
        testing.AddArgument(join(sys.argv[x+1], "SaveTransparentImages.png"))

result = testing.RegressionTest(testimage, 10)
if result == testing.DO_INTERACTOR:
    sys.exit(0)
elif result == testing.NOT_RUN:
    sys.exit(125)
elif result == testing.FAILED:
    raise RuntimeError("test failed!")
