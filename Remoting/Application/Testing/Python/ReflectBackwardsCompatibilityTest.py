import paraview
paraview.compatibility.major = 5
paraview.compatibility.minor = 13

from paraview.simple import *

sphere = Sphere()

sphereReflection = Reflect(Input=sphere)
assert(type(sphereReflection).__name__ == "AxisAlignedReflectionFilter")

sphereReflection.Plane = 7
sphereReflection.Center = 2

assert(sphereReflection.ReflectionPlane.Normal == [0.0, 1.0, 0.0])
assert(sphereReflection.ReflectionPlane.Origin == [2.0, 2.0, 2.0])

sphereReflection.Plane = 'Z Min'
assert(sphereReflection.PlaneMode == 'Z Min')

sphereReflection.PlaneMode = 2
assert(sphereReflection.Plane == 'Y Min')

sphereReflection.Plane = 2
assert(sphereReflection.PlaneMode == 'Z Min')

sphereReflection.Plane = 8
assert(sphereReflection.ReflectionPlane.Normal == [0.0, 0.0, 1.0])

assert(not sphereReflection.ReflectAllInputArrays)
sphereReflection.FlipAllInputArrays = True
assert(sphereReflection.ReflectAllInputArrays)
assert(sphereReflection.FlipAllInputArrays)
