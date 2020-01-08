from paraview.simple import *

from paraview.vtk.vtkTestingRendering import vtkTesting
from os.path import join
import sys

testing = vtkTesting()
for arg in sys.argv:
    testing.AddArgument(arg)
for x in range(1, len(sys.argv)):
    if sys.argv[x] == "-B" and len(sys.argv) > (x+1):
        testing.AddArgument("-V")
        testing.AddArgument(join(sys.argv[x+1], "TestStereoSaveScreenshot.png"))

Sphere()
r = Show()
r.Representation = "Surface With Edges"
Render()

fname = join(testing.GetTempDirectory(), "TestStereoSaveScreenshot.png")
SaveScreenshot(fname, ImageResolution=[800, 800])

result = testing.RegressionTest(fname, 10)
if result == testing.DO_INTERACTOR:
    sys.exit(0)
elif result == testing.NOT_RUN:
    sys.exit(125)
elif result == testing.FAILED:
    raise RuntimeError("test failed!")
