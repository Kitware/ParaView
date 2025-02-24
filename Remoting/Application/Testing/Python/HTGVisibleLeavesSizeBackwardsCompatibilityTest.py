import paraview
paraview.compatibility.major = 5
paraview.compatibility.minor = 14

from paraview.simple import *

hyperTreeGridRandom = HyperTreeGridRandom()

hyperTreeGridVisibleLeavesSize = HyperTreeGridVisibleLeavesSize(Input=hyperTreeGridRandom)
assert(type(hyperTreeGridVisibleLeavesSize).__name__ == "HyperTreeGridGenerateFields")

hyperTreeGridVisibleLeavesSize.ValidCellArrayName = "Valid"
assert hyperTreeGridVisibleLeavesSize.ValidCellArrayName == "Valid"

hyperTreeGridVisibleLeavesSize.CellSizeArrayName = "Size"
assert hyperTreeGridVisibleLeavesSize.CellSizeArrayName == "Size"
