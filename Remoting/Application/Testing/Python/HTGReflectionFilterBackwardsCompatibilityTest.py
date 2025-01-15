import paraview
paraview.compatibility.major = 5
paraview.compatibility.minor = 13

from paraview.simple import *

hyperTreeGridRandom = HyperTreeGridRandom()

hyperTreeGridAxisReflection = HyperTreeGridAxisReflection(Input=hyperTreeGridRandom)
assert(type(hyperTreeGridAxisReflection).__name__ == "AxisAlignedReflectionFilter")

hyperTreeGridAxisReflection.PlaneNormal = 'Z Axis'
assert(hyperTreeGridAxisReflection.ReflectionPlane.Normal == [0.0, 0.0, 1.0])

hyperTreeGridAxisReflection.PlaneNormal = 7
hyperTreeGridAxisReflection.PlanePosition = 2

assert(hyperTreeGridAxisReflection.ReflectionPlane.Normal == [0.0, 1.0, 0.0])
assert(hyperTreeGridAxisReflection.ReflectionPlane.Origin == [2.0, 2.0, 2.0])
