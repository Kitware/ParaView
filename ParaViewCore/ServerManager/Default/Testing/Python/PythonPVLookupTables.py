from paraview.simple import *
from paraview import smtesting

smtesting.ProcessCommandLineArguments()
lutfile='%s/testlut.xml'%(smtesting.TempDir)
testlut="""<ColorMaps>
<ColorMap name="testlut" space="RGB" indexedLookup="false">
  <Point x="1.86486" o="0" r="0.0392157" g="0.0392157" b="0.94902"/>
  <Point x="70.1081" o="0.6954" r="0" g="1" b="0"/>
  <Point x="100" o="1" r="0.94902" g="0.94902" b="0.0392157"/>
  <NaN r="1" g="0" b="0"/>
</ColorMap>
</ColorMaps>"""
f = open(lutfile,'w')
f.write(testlut)
f.close()

w = Wavelet()
wa = w.PointData.GetArray('RTData')

o = Outline(w)
Show(o)

s = Slice(w)
s.SliceType.Normal = [0.0, 0.0, 1.0]

r = Show(s)
r.ColorArrayName='RTData'
r.Representation='Surface'

v = GetRenderView()
v.CameraViewUp = [0,1,0]
v.CameraPosition = [-32,32,60]
v.CameraClippingRange = [33, 110]
v.UseGradientBackground = 1
v.Background2 = [0.0, 0.0, 0.16471]
v.Background = [0.3216, 0.3412, 0.4314]
v.CenterAxesVisibility = 0

# verify default laoding
if len(GetLookupTableNames())<1:
  raise smtesting.TestError('Failed to load the default LUTs.')

# exercsie the simple loader
if not LoadLookupTable(lutfile):
  raise smtesting.TestError('Failed to load the testlut file.')

names = GetLookupTableNames()
if 'testlut' not in names:
  raise smtesting.TestError('Failed to parse the testlut lut.')

# exercise simple assignment
print
print 'Rendering with %d LUTs'%(len(names))
print 'The available LUTs are:'
print names
print
for name in names:
  r.LookupTable=AssignLookupTable(wa,name)
  Render()

# choose one of them for the baseline
r.LookupTable=AssignLookupTable(wa,'Cold and Hot')
ren = Render()

if not smtesting.DoRegressionTesting(ren.SMProxy):
   raise smtesting.TestError('Image comparison failed.')
