import paraview
paraview.compatibility.major = 5
paraview.compatibility.minor = 14

from paraview.simple import *

hyperTreeGridRandom = HyperTreeGridRandom()

hyperTreeGridGhostCellsGenerator = HyperTreeGridGhostCellsGenerator(Input=hyperTreeGridRandom)
assert(type(hyperTreeGridGhostCellsGenerator).__name__ == "GhostCells")
