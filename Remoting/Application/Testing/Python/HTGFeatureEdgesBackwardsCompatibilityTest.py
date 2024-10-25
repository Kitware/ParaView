import paraview
paraview.compatibility.major = 5
paraview.compatibility.minor = 14

from paraview.simple import *

hyperTreeGridRandom = HyperTreeGridRandom()

hyperTreeGridFeatureEdges = HyperTreeGridFeatureEdges(Input=hyperTreeGridRandom)
print('------------', hyperTreeGridFeatureEdges)
assert(type(hyperTreeGridFeatureEdges).__name__ == "FeatureEdges")
