from paraview.simple import *
from paraview.selection import *

from paraview import smtesting

smtesting.ProcessCommandLineArguments()

s = Sphere()
c = Cone(Resolution=10)
GroupDatasets(Input=[s, c])
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
    if numPoints is not None:
        assert numPoints == info.GetNumberOfPoints()
    if numCells is not None:
        assert numCells == info.GetNumberOfCells()
    del es


SetActiveSource(s)
Show(s)
Render()

# Rectangle selection
SelectSurfacePoints(Rectangle=[0, 0, 220, 220], View=RenderView1)
CheckSelection(numPoints=10, numCells=10)

SetActiveSource(s)
SelectSurfaceCells(Rectangle=[0, 0, 220, 220], View=RenderView1)
CheckSelection(numPoints=19, numCells=24)

# Polygon selection. Use active view instead of passing it in.
SetActiveSource(s)
SelectSurfacePoints(Polygon=[0, 0, 0, 220, 220, 220])
CheckSelection(numPoints=7, numCells=7)

SetActiveSource(s)
SelectSurfaceCells(Polygon=[0, 0, 0, 220, 220, 220])
CheckSelection(numPoints=16, numCells=16)

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

# Composite data ids selection
Hide(s)
SetActiveSource(g)
Show(g)
Render()

# Should make selection within the whole dataset (0 is the root block id)
SelectCompositeDataIDs(IDs=[0, 0, 0], FieldType="POINT")
CheckSelection(numPoints=2, numCells=2)

Hide(s)
SetActiveSource(g)
Show(g)
Render()

# Should make selection within the first subblock
SelectCompositeDataIDs(IDs=[1, 0, 0], FieldType="POINT")
CheckSelection(numPoints=1, numCells=1)

Hide(s)
SetActiveSource(g)
Show(g)
Render()

# Should make selection within the second subblock
SelectCompositeDataIDs(IDs=[2, 0, 0], FieldType="POINT")
CheckSelection(numPoints=1, numCells=1)

try:
    SelectSurfacePoints(Rectangle=[0, 0], Polygon=[0, 0])
    sys.exit(1)  # Should not get here
except:
    pass  # Exception thrown as expected
