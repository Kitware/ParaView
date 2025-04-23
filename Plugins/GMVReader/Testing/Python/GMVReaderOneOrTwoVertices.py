#### Verify that Merge Blocks produces polydata output when inputs are polydata
from paraview.simple import *
from paraview import servermanager as sm
from paraview import smtesting
import os;

# This test makes sure that the GMV reader can read vertices and lines.

smtesting.ProcessCommandLineArguments()

GMVDir = os.path.join(smtesting.DataDir, "Plugins", "GMVReader", "Testing", "Data", "GMV")

# load plugin
LoadDistributedPlugin('GMVReader', ns=globals())

# create a new 'GMV Reader'
one_vertexgmv = GMVReader(registrationName='one_vertex.gmv',
        FileNames=[os.path.join(GMVDir, "one_vertex.gmv")])
one_vertexgmv.CellArrayStatus = ['material id']

assert (sm.Fetch(one_vertexgmv).GetBlock(1).GetNumberOfVerts() == 3)

# create a new 'GMV Reader'
two_vertexgmv = GMVReader(registrationName='two_vertex.gmv',
        FileNames=[os.path.join(GMVDir, "two_vertex.gmv")])
two_vertexgmv.CellArrayStatus = ['material id']

assert (sm.Fetch(two_vertexgmv).GetBlock(1).GetNumberOfLines() == 2)
