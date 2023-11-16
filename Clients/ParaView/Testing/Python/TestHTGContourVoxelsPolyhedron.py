from paraview.vtk.util.misc import vtkGetTempDir
from paraview.vtk.vtkTestingRendering import vtkTesting
from os.path import join
import sys
from paraview.simple import *

"""
This rendering test ensures that the HTG contour using the "Voxels" strategy
is the same as the decomposed polyhedra contour for a 2D slice.
"""

# Create the first renderview
renderView1 = CreateView('RenderView')
renderView1.ViewSize = [678, 932]
renderView1.InteractionMode = '2D'
renderView1.AxesGrid = 'Grid Axes 3D Actor'
renderView1.OrientationAxesVisibility = 0
renderView1.CenterOfRotation = [-3.89453125, -0.1171875, 0.0]
renderView1.StereoType = 'Crystal Eyes'
renderView1.CameraPosition = [-3.89453125, -0.1171875, 57.50021284863458]
renderView1.CameraFocalPoint = [-3.89453125, -0.1171875, 0.0]
renderView1.CameraFocalDisk = 1.0
renderView1.CameraParallelScale = 15.1225198715785

# Create the second renderview
renderView2 = CreateView('RenderView')
renderView2.ViewSize = [678, 932]
renderView2.AxesGrid = 'Grid Axes 3D Actor'
renderView2.OrientationAxesVisibility = 0
renderView2.CenterOfRotation = [-3.89453125, -0.1171875, 0.0]
renderView2.StereoType = 'Crystal Eyes'
renderView2.CameraPosition = [-3.89453125, -0.1171875, 39.27341906197292]
renderView2.CameraFocalPoint = [-3.89453125, -0.1171875, 0.0]
renderView2.CameraFocalDisk = 1.0
renderView2.CameraParallelScale = 15.1225198715785

# create new layout object 'Layout #1'
layout1 = CreateLayout(name='Layout #1')
layout1.SplitHorizontal(0, 0.500000)
layout1.AssignView(1, renderView2)
layout1.AssignView(2, renderView1)
layout1.SetSize(1357, 932)

# create a new 'Hyper Tree Grid (Random)'
hyperTreeGridRandom = HyperTreeGridRandom(registrationName='HyperTreeGridRandom')
hyperTreeGridRandom.Seed = 42
hyperTreeGridRandom.Dimensions = [3, 3, 3]

# create a new 'Slice'
sliceZ = Slice(registrationName='SliceZ', Input=hyperTreeGridRandom)
sliceZ.SliceType = 'Plane'
sliceZ.HyperTreeGridSlicer = 'Axis Aligned Plane'
sliceZ.SliceOffsetValues = [0.0]
sliceZ.PointMergeMethod = 'Uniform Binning'
sliceZ.HyperTreeGridSlicer.Normal = [0.0, 0.0, 1.0]

# create a new 'Contour' using Voxels strategy (default)
contourVoxels = Contour(registrationName='Contour-Voxels', Input=sliceZ)
contourVoxels.ContourBy = ['CELLS', 'Depth']
contourVoxels.Isosurfaces = [3.5, 4.5]
contourVoxels.PointMergeMethod = 'Uniform Binning'

# Show Voxel contour
contourVoxelsDisplay = Show(contourVoxels, renderView1, 'GeometryRepresentation')
depthTF2D = GetTransferFunction2D('Depth')
depthLUT = GetColorTransferFunction('Depth')
depthLUT.TransferFunction2D = depthTF2D
depthLUT.RGBPoints = [0.0, 0.231373, 0.298039, 0.752941, 2.5, 0.865003, 0.865003, 0.865003, 5.0, 0.705882, 0.0156863, 0.14902]
depthLUT.ScalarRangeInitialized = 1.0
depthLUT.VectorMode = 'Component'

contourVoxelsDisplay.Representation = 'Surface'
contourVoxelsDisplay.ColorArrayName = ['POINTS', 'Depth']
contourVoxelsDisplay.LookupTable = depthLUT
contourVoxelsDisplay.ScaleTransferFunction.Points = [2.5, 0.0, 0.5, 0.0, 2.50048828125, 1.0, 0.5, 0.0]
contourVoxelsDisplay.OpacityTransferFunction.Points = [2.5, 0.0, 0.5, 0.0, 2.50048828125, 1.0, 0.5, 0.0]

# create a new 'Contour' using the decomposed polyhedra strategy
contourPolyhedraConcave = Contour(registrationName='Contour-PolyhedraConcave', Input=sliceZ)
contourPolyhedraConcave.ContourBy = ['CELLS', 'Depth']
contourPolyhedraConcave.Contourstrategy3D = 'Use Decomposed Polyhedra'
contourPolyhedraConcave.Isosurfaces = [3.5, 4.5]
contourPolyhedraConcave.PointMergeMethod = 'Uniform Binning'

# Show Polyhedron contour
contourPolyhedraConcaveDisplay = Show(contourPolyhedraConcave, renderView1, 'GeometryRepresentation')
depthTF2D = GetTransferFunction2D('Depth')

contourPolyhedraConcaveDisplay.Representation = 'Surface'
contourPolyhedraConcaveDisplay.ColorArrayName = ['POINTS', 'Depth']
contourPolyhedraConcaveDisplay.LookupTable = depthLUT
contourPolyhedraConcaveDisplay.ScaleTransferFunction.Points = [2.5, 0.0, 0.5, 0.0, 2.50048828125, 1.0, 0.5, 0.0]
contourPolyhedraConcaveDisplay.OpacityTransferFunction.Points = [2.5, 0.0, 0.5, 0.0, 2.50048828125, 1.0, 0.5, 0.0]


# Save screenshot of contour using voxels
testimageVoxels = join(vtkGetTempDir(), "TestHTGContourVoxels.png")
SaveScreenshot(testimageVoxels, renderView1, saveInBackground = True, ImageResolution=(300, 300),
    TransparentBackground=1)

# Save screenshot of contour using decomposed polyhedra
testImagePolyhedron = join(vtkGetTempDir(), "TestHTGContourPolyhedron.png")
SaveScreenshot(testImagePolyhedron, renderView1, saveInBackground = True, ImageResolution=(300, 300),
    TransparentBackground=1)

servermanager.vtkRemoteWriterHelper.Wait(testimageVoxels)
servermanager.vtkRemoteWriterHelper.Wait(testImagePolyhedron)

# Both images must match exactly
testing = vtkTesting()

# Copy every argv from script launch command line to testing function command line
for arg in sys.argv:
    testing.AddArgument(arg)
for x in range(1, len(sys.argv)):
    if sys.argv[x] == "-B" and len(sys.argv) > (x+1):
        # Add the baseline as a -V argument
        testing.AddArgument("-V")
        testing.AddArgument(join(sys.argv[x+1], "TestHTGContourVoxelsPolyhedron.png"))

resultVoxels = testing.RegressionTest(testimageVoxels, 10)
resultPolyhedron = testing.RegressionTest(testImagePolyhedron, 10)

if resultVoxels == testing.DO_INTERACTOR and resultPolyhedron == testing.DO_INTERACTOR:
    sys.exit(0)
elif resultVoxels == testing.NOT_RUN or resultPolyhedron == testing.NOT_RUN:
    sys.exit(125)
elif resultVoxels == testing.FAILED or resultPolyhedron == testing.FAILED:
    raise RuntimeError("test failed!")
