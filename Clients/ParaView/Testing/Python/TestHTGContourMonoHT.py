from paraview.vtk.util.misc import vtkGetTempDir
from paraview.vtk.vtkTestingRendering import vtkTesting
from os.path import join
import sys
from paraview.simple import *

"""
This file is a rendering test of HTG Contour using polyhedron,
in the case of a one cell grid (see issue Gitlab issue #22324)
"""

# Create first new 'Render View'
renderView1 = CreateView('RenderView')
renderView1.ViewSize = [672, 679]
renderView1.AxesGrid = 'Grid Axes 3D Actor'
renderView1.StereoType = 'Crystal Eyes'
renderView1.CameraPosition = [15.633216658674037, -17.535848125900205, 11.894483975879004]
renderView1.CameraFocalPoint = [-2.9303151553033495, 3.286947443149022, -2.229521116480663]
renderView1.CameraViewUp = [0.5541430882474583, -0.06868348660868764, -0.8295830376851037]
renderView1.CameraFocalDisk = 1.0
renderView1.CameraParallelScale = 38.21686297990684

# create new layout object 'Layout #1'
layout1 = CreateLayout(name='Layout #1')
layout1.SplitHorizontal(0, 0.500000)
layout1.AssignView(1, renderView1)
layout1.SetSize(1345, 679)

# create a new 'Hyper Tree Grid (Random)'
hyperTreeGridRandom = HyperTreeGridRandom(registrationName='HyperTreeGridRandom')
hyperTreeGridRandom.Seed = 42
hyperTreeGridRandom.Dimensions = [2, 2, 2]

# create a new 'Contour' using polyhedra strategy
contourPolyhedraConcave = Contour(registrationName='Contour-PolyhedraConcave', Input=hyperTreeGridRandom)
contourPolyhedraConcave.ContourBy = ['CELLS', 'Depth']
contourPolyhedraConcave.Contourstrategy3D = 'Use Decomposed Polyhedra'
contourPolyhedraConcave.Isosurfaces = [3.5, 4.5]
contourPolyhedraConcave.PointMergeMethod = 'Uniform Binning'

# Setup the view for the contour
contourPolyhedraConcaveDisplay = Show(contourPolyhedraConcave, renderView1, 'GeometryRepresentation')
depthTF2D = GetTransferFunction2D('Depth')
depthLUT = GetColorTransferFunction('Depth')
depthLUT.TransferFunction2D = depthTF2D
depthLUT.RGBPoints = [0.0, 0.231373, 0.298039, 0.752941, 2.5, 0.865003, 0.865003, 0.865003, 5.0, 0.705882, 0.0156863, 0.14902]
depthLUT.ScalarRangeInitialized = 1.0
depthLUT.VectorMode = 'Component'
contourPolyhedraConcaveDisplay.Representation = 'Surface'
contourPolyhedraConcaveDisplay.ColorArrayName = ['POINTS', 'Depth']
contourPolyhedraConcaveDisplay.LookupTable = depthLUT
contourPolyhedraConcaveDisplay.ScaleTransferFunction.Points = [2.5, 0.0, 0.5, 0.0, 2.50048828125, 1.0, 0.5, 0.0]
contourPolyhedraConcaveDisplay.OpacityTransferFunction.Points = [2.5, 0.0, 0.5, 0.0, 2.50048828125, 1.0, 0.5, 0.0]


# Save screenshot of contour using polyhedron
testImagePolyhedron = join(vtkGetTempDir(), "TestHTGContourMonoHT.png")
SaveScreenshot(testImagePolyhedron, renderView1, saveInBackground = True, ImageResolution=(300, 300),
    TransparentBackground=1)
servermanager.vtkRemoteWriterHelper.Wait(testImagePolyhedron)


testing = vtkTesting()
for arg in sys.argv:
    testing.AddArgument(arg)
for x in range(1, len(sys.argv)):
    if sys.argv[x] == "-B" and len(sys.argv) > (x+1):
        testing.AddArgument("-V")
        testing.AddArgument(join(sys.argv[x+1], "TestHTGContourMonoHT.png"))

resultPolyhedron = testing.RegressionTest(testImagePolyhedron, 10)


if resultPolyhedron == testing.DO_INTERACTOR:
    sys.exit(0)
elif resultPolyhedron == testing.NOT_RUN:
    sys.exit(125)
elif resultPolyhedron == testing.FAILED:
    raise RuntimeError("test failed!")
