import sys
if len(sys.argv) != 2:
    print "command is 'python <vtk file>'"
    sys.exit(1)

import vtk

r = vtk.vtkXMLPPolyDataReader()
r.SetFileName(sys.argv[1])
r.Update()

g = r.GetOutput()

if g.GetNumberOfPoints() != 441 or g.GetNumberOfCells() != 800:
    print 'Output grid is incorrect. The number of points is', g.GetNumberOfPoints(), \
        'but should be 441 and the number of cells is', g.GetNumberOfCells(), \
        'but should be 800.'
    sys.exit(1)
