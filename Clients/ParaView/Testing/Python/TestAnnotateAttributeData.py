from paraview.simple import *

s = Sphere()
e = Elevation(LowPoint=[-0.5,-0.5,-0.5],
              HighPoint=[0.5, 0.5, 0.5])
UpdatePipeline()
a = AnnotateAttributeData(
        Prefix="Hello: ",
        SelectInputArray= ('POINTS', 'Elevation'),
        ElementId=0,
        ProcessId=0
        )
UpdatePipeline()
annotatedValue = a.GetClientSideObject().GetComputedAnnotationValue()
print(annotatedValue)
assert annotatedValue[0:14] == "Hello: 0.66666"


text1 = Text()
annotateAttributeData1 = AnnotateAttributeData(Input=text1)
annotateAttributeData1.SelectInputArray = ('ROWS', 'Text')

UpdatePipeline()
annotatedValue = annotateAttributeData1.GetClientSideObject().GetComputedAnnotationValue()
print(annotatedValue)
assert annotatedValue == "Value is: Text"
