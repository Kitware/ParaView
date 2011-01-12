from paraview.simple import *
import time

session = servermanager.ActiveSession
pxm = servermanager.ProxyManager()

sphere = Sphere()
sphere.UpdateVTKObjects();

filter = NormalGlyphs()
filter.Input = sphere
filter.GlyphScaleFactor = 0.1
filter.UpdateVTKObjects()

Show(sphere)
Show(filter)
view = Render()
ResetCamera()
Render()
view = GetRenderView()
camera = view.GetActiveCamera()

for i in xrange(30):
    time.sleep(0.1)
    camera.Azimuth(10)
    Render()

print "--- Loading done ---"
