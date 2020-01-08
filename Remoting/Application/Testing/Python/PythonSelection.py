from paraview.simple import *
from paraview.selection import *

from paraview import smtesting
smtesting.ProcessCommandLineArguments()

s = Sphere()
c =Cone(Resolution=10)
GroupDatasets(Input=[s,c])
g = GenerateIds()

sel = SelectCells("CellIds > 4")

e = ExtractSelection(Selection=sel)
r = Show()
r.ColorArrayName = None
RenderView1 = Render()

if not smtesting.DoRegressionTesting(RenderView1.SMProxy):
  # This will lead to VTK object leaks.
  import sys
  sys.exit(1)

counter = 0
def CheckSelection(numPoints=None, numCells=None):
  global counter
  Render()
  SaveScreenshot("tmp%d.png" % counter)
  print("Saving %s" % ("tmp%d.png" % counter))
  counter = counter + 1
  es = ExtractSelection()
  es.UpdatePipeline()
  info = es.GetDataInformation()
  print("Points and cells:", info.GetNumberOfPoints(), info.GetNumberOfCells())
  if numPoints:
    assert numPoints == info.GetNumberOfPoints()
  if numCells:
    assert numCells == info.GetNumberOfCells()
  del es

SetActiveSource(s)
Show(s)
Render()

# Rectangle selection
SelectSurfacePoints(Rectangle=[100, 100, 200, 200], View=RenderView1)
CheckSelection(numPoints=8, numCells=8)

SetActiveSource(s)
SelectSurfaceCells(Rectangle=[100, 100, 200, 200], View=RenderView1)
CheckSelection(numPoints=18, numCells=20)

# Polygon selection. Use active view instead of passing it in.
SetActiveSource(s)
SelectSurfacePoints(Polygon=[100, 100, 100, 200, 200, 200])
CheckSelection(numPoints=6, numCells=6)

SetActiveSource(s)
SelectSurfaceCells(Polygon=[100, 100, 100, 200, 200, 200])
CheckSelection(numPoints=15, numCells=14)

# Rectangle selection through
HideAll()
Show(s)
SetActiveSource(s)
ClearSelection(s)
SelectPointsThrough(Rectangle=[0, 0, 300, 300])
CheckSelection(numPoints=46, numCells=46)

SetActiveSource(s)
SelectCellsThrough(Rectangle=[0, 0, 300, 300])
CheckSelection(numPoints=50, numCells=96)

# Block selection
Hide(s)
SetActiveSource(g)
Show(g)
Render()
SelectSurfaceBlocks(Rectangle=[100, 100, 200, 200])
CheckSelection(numPoints=61, numCells=107)

try:
  SelectSurfacePoints(Rectangle=[0, 0], Polygon=[0, 0])
  sys.exit(1) # Should not get here
except:
  pass # Exception thrown as expected
