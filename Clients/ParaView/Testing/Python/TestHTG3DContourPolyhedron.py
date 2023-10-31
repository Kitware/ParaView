from paraview.vtk.util.misc import vtkGetTempDir
from paraview.vtk.vtkTestingRendering import vtkTesting
from os.path import join
import sys
from paraview.simple import *

"""
This file tests that the HTG contour using decomposed polyhedra, shows the right surface,
being more precise than the voxel contour algorithm, which should give a different output.
"""


# Create the first renderview
renderView1 = CreateView('RenderView')
renderView1.ViewSize = [629, 684]
renderView1.AxesGrid = 'Grid Axes 3D Actor'
renderView1.StereoType = 'Crystal Eyes'
renderView1.CameraPosition = [24.018791358918232, 0.3378148916555779, -10.787294582047418]
renderView1.CameraFocalPoint = [-4.502120700288218, -0.06332056404759372, 2.021987764172411]
renderView1.CameraViewUp = [-0.3454419719658508, 0.5619913796883795, -0.7515554092415916]
renderView1.CameraFocalDisk = 1.0
renderView1.CameraParallelScale = 38.21686297990684

# Create a new 'Render View'
renderView2 = CreateView('RenderView')
renderView2.ViewSize = [630, 684]
renderView2.AxesGrid = 'Grid Axes 3D Actor'
renderView2.StereoType = 'Crystal Eyes'
renderView2.CameraPosition = [24.018791358918232, 0.3378148916555779, -10.787294582047418]
renderView2.CameraFocalPoint = [-4.502120700288218, -0.06332056404759372, 2.021987764172411]
renderView2.CameraViewUp = [-0.3454419719658508, 0.5619913796883795, -0.7515554092415916]
renderView2.CameraFocalDisk = 1.0
renderView2.CameraParallelScale = 38.21686297990684

# create new layout object 'Layout #1'
layout1 = CreateLayout(name='Layout #1')
layout1.SplitHorizontal(0, 0.500000)
layout1.AssignView(1, renderView2)
layout1.AssignView(2, renderView1)
layout1.SetSize(1260, 684)

# create a new 'Hyper Tree Grid (Random)'
hyperTreeGridRandom = HyperTreeGridRandom(registrationName='HyperTreeGridRandom')
hyperTreeGridRandom.Seed = 42
hyperTreeGridRandom.Dimensions = [3, 3, 3]

# create a new 'Contour'
contourPolyhedraConcave = Contour(registrationName='Contour-PolyhedraConcave', Input=hyperTreeGridRandom)
contourPolyhedraConcave.ContourBy = ['CELLS', 'Depth']
contourPolyhedraConcave.Contourstrategy3D = 'Use Decomposed Polyhedra'
contourPolyhedraConcave.Isosurfaces = [3.5, 4.5]
contourPolyhedraConcave.PointMergeMethod = 'Uniform Binning'

# show data from contourPolyhedraConcave
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
testImagePolyhedron = join(vtkGetTempDir(), "TestHTG3DContourPolyhedron.png")
SaveScreenshot(testImagePolyhedron, renderView1, saveInBackground = True, ImageResolution=(300, 300),
    TransparentBackground=1)
servermanager.vtkRemoteWriterHelper.Wait(testImagePolyhedron)


testing = vtkTesting()
for arg in sys.argv:
    testing.AddArgument(arg)
for x in range(1, len(sys.argv)):
    if sys.argv[x] == "-B" and len(sys.argv) > (x+1):
        testing.AddArgument("-V")
        testing.AddArgument(join(sys.argv[x+1], "TestHTG3DContourPolyhedron.png"))

resultPolyhedron = testing.RegressionTest(testImagePolyhedron, 10)


if resultPolyhedron == testing.DO_INTERACTOR:
    sys.exit(0)
elif resultPolyhedron == testing.NOT_RUN:
    sys.exit(125)
elif resultPolyhedron == testing.FAILED:
    raise RuntimeError("test failed!")
