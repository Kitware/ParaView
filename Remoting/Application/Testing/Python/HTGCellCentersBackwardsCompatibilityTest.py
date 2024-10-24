import paraview
paraview.compatibility.major = 5
paraview.compatibility.minor = 14

from paraview.simple import *

hyperTreeGridRandom = HyperTreeGridRandom()

hyperTreeGridCellCenters = HyperTreeGridCellCenters(Input=hyperTreeGridRandom)
assert(type(hyperTreeGridCellCenters).__name__ == "CellCenters")
