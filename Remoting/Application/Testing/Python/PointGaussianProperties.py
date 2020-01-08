
import os, sys
from paraview import simple
from paraview.modules.vtkRemotingViews import *
from vtk import *
from paraview import smtesting

if sys.version_info > (3,):
    xrange = range

NUM_POINTS = 5

### Create the geometry
points = vtkPoints()
points.SetNumberOfPoints(NUM_POINTS)

for i in xrange(NUM_POINTS):
    points.SetPoint(i, i, 0.0, 0.0)

verts = vtkCellArray()
verts.InsertNextCell(NUM_POINTS)

for i in xrange(NUM_POINTS):
    verts.InsertCellPoint(i)

polyData = vtkPolyData()
polyData.SetPoints(points)
polyData.SetVerts(verts)

### Create some point data
pointData = polyData.GetPointData();

# Set up a data array containing the point ordinal values
ordinalArrayName = "Ordinal";
ordinalRange = [ 0, NUM_POINTS - 1 ]
ordinalArray = vtkDoubleArray()
ordinalArray.SetName(ordinalArrayName);
ordinalArray.SetNumberOfComponents(1);
ordinalArray.SetNumberOfTuples(NUM_POINTS);

for i in xrange(NUM_POINTS):
    ordinalArray.SetTuple1(i, i)

pointData.AddArray(ordinalArray)

source = simple.TrivialProducer()
source.GetClientSideObject().SetOutput(polyData)

# create a calculator to compute distance
calculator1 = simple.Calculator(Input=source)
calculator1.ResultArrayName = 'Distance'
calculator1.Function = 'mag(coords)'

# create another calculator to compute inverse distance
calculator2 = simple.Calculator(Input=calculator1)
calculator2.ResultArrayName = 'Inverse Distance'
calculator2.Function = '%s-Distance' % str(NUM_POINTS - 1)

# Get the representation
rep = simple.Show(calculator2)

# Set up coloring by one array
rep.Representation = 'Point Gaussian'
simple.ColorBy(rep, ('POINTS', 'Ordinal'))
vtkSMPVRepresentationProxy.RescaleTransferFunctionToDataRange(rep.SMProxy, 'Ordinal', 0, False)

# Set up sizing by another array
scaleTransferFunction = simple.CreatePiecewiseFunction(Points=[0.0, 0.05, 0.5, 0.0, NUM_POINTS - 1, 0.15, 0.5, 0.0])
rep.ScaleTransferFunction = scaleTransferFunction
rep.SetScaleArray = 'Distance'
rep.ScaleByArray = 1
rep.GaussianRadius = 1

# And finally, set up opacity by a third array
opacityTransferFunction = simple.CreatePiecewiseFunction(Points=[0.0, 0.2, 0.5, 0.0, NUM_POINTS - 1, 1.0, 0.5, 0.0])
rep.OpacityTransferFunction = opacityTransferFunction
rep.OpacityArray = 'Inverse Distance'
rep.OpacityByArray = 1

# Now set a custom shader snippet
rep.CustomShader = '''
//VTK::Color::Impl
float dist = dot(offsetVCVSOutput.xy,offsetVCVSOutput.xy);
if (dist > 9.0) {
  discard;
}
'''
rep.CustomTriangleScale = 3
rep.ShaderPreset = "Custom"

# Now render, configure the view, and re-render
renderView = simple.Render()

renderView.CenterOfRotation         = [ (NUM_POINTS - 1) / 2.0 , 0, 0              ]
renderView.CameraPosition           = [ (NUM_POINTS - 1) / 2.0 , 0, NUM_POINTS * 2 ]
renderView.CameraFocalPoint         = [ (NUM_POINTS - 1) / 2.0 , 0, 0              ]
renderView.CameraViewAngle          = 30.0
renderView.CameraParallelProjection = 0

simple.Render(renderView)

if not smtesting.DoRegressionTesting(renderView.SMProxy):
    raise smtesting.TestError('Image comparison failed.')
