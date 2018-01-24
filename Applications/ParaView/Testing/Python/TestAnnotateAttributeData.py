from paraview.simple import *

s = Sphere()
e = Elevation(LowPoint=[-0.5,-0.5,-0.5],
              HighPoint=[0.5, 0.5, 0.5])
UpdatePipeline()
a = AnnotateAttributeData(
        Prefix="Hello: ",
        ArrayName= "Elevation",
        ArrayAssociation="Point Data",
        ElementId=0,
        ProcessId=0
        )
UpdatePipeline()
annotatedValue = a.GetClientSideObject().GetComputedAnnotationValue()
print(annotatedValue)
assert annotatedValue == "Hello: 0.666667" or annotatedValue == "Hello: 0.6666667"
