import sys
if len(sys.argv) != 2:
    print("command is 'python <vtk file>'")
    sys.exit(1)

from paraview.simple import *

proxy = OpenDataFile(sys.argv[1])
r = proxy.GetClientSideObject()
r.Update()

g = r.GetOutput()

if g.GetNumberOfPoints() != 441 or g.GetNumberOfCells() != 800:
    print('Output grid is incorrect. The number of points is %d '\
        'but should be 441 and the number of cells is %d ' \
        'but should be 800.' % (g.GetNumberOfPoints(), g.GetNumberOfCells()))
    sys.exit(1)
else:
  print("All's good!!!!")
