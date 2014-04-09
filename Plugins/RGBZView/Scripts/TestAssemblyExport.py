# -----------------------------------------------------------------------------
# ParaView Python - Path setup
# -----------------------------------------------------------------------------

import sys, time
pv_path = '/Users/seb/work/code/ParaView/build-ninja'
# sys.path.append('%s/lib' % pv_path)
# sys.path.append('%s/lib/site-packages' % pv_path)

from paraview.simple import *

# LoadPlugin(pv_path + '/lib/libRGBZView.dylib')

s = Sphere()
c = Cone()

view = CreateView("RGBZView")
view.CompositeDirectory = '/Users/seb/Desktop/composite-new'

rs = Show(s,view)
rc = Show(c,view)

view.ResetClippingBounds()
view.FreezeGeometryBounds()

Render(view)

view.ComputeZOrdering()
view.WriteImages()
view.WriteComposite()

#view.UpdatePropertyInformation()
#print view.GetProperty('RepresentationCodes')
