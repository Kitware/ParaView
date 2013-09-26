from paraview.simple import *
import sys
from paraview import smtesting

sphere = Sphere()
sphere.PhiResolution = 20
sphere.ThetaResolution = 20

clientsphere = servermanager.Fetch(sphere)
if clientsphere.GetNumberOfPolys() != 720L:
    raise smtesting.Error('Test failed: Problem fetching polydata.')

elev = Elevation(sphere)
mm = MinMax(None)
mm.Operation = "MIN"

mindata = servermanager.Fetch(elev, mm, mm)

if mindata.GetPointData().GetNumberOfArrays() != 2:
    raise smtesting.Error('Test failed: Wrong number of arrays.')

array = mindata.GetPointData().GetArray('Elevation')

if array.GetTuple1(0) != 0.0:
    raise smtesting.Error('Test failed: Bad array value.')

rep = Show(elev)
ai = elev.PointData[1]
if ai.GetName() != 'Elevation':
    pd = elev.GetPointDataInformation()
    ai = pd.GetArray('Elevation')

rng = ai.GetRange()
rep.LookupTable = MakeBlueToRedLT(rng[0], rng[1])
rep.ColorArrayName = 'Elevation'
rep.ColorAttributeType = 'POINT_DATA'

camera = GetActiveCamera()
camera.Elevation(45)

ren = Render()

if not smtesting.DoRegressionTesting(ren.SMProxy):
    raise smtesting.Error('Image comparison failed.')

