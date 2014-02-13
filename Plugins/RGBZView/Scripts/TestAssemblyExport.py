# -----------------------------------------------------------------------------
# ParaView Python - Path setup
# -----------------------------------------------------------------------------

# import sys
pv_path = '/Users/seb/work/code/ParaView/build-ninja'
# sys.path.append('%s/lib' % pv_path)
# sys.path.append('%s/lib/site-packages' % pv_path)

from paraview.simple import *

LoadPlugin(pv_path + '/lib/libRGBZView.dylib')

s = Sphere()
c = Cone()

view = CreateView("RGBZView")

rs = Show(s,view)
rc = Show(c,view)

view.ResetClippingBounds()
view.FreezeGeometryBounds()

# Export scene composition
repList = view.Representations
fileNames = '/tmp/export-%d.%s'
fileToProcess = []

for activeRepIdx in range(len(repList) + 1):
    for i in range(len(repList)):
        repList[i].Visibility = 0
    if activeRepIdx > 0:
        repList[activeRepIdx-1].Visibility = 1
    view.FileName = fileNames % (activeRepIdx, 'vtk')
    fileToProcess.append(view.FileName)
    Render(view)

# Process generated data for composition
pxm = servermanager.ProxyManager()
assemblyExporter = servermanager._getPyProxy(pxm.NewProxy('misc', 'AssemblyGenerator'))

assemblyExporter.DestinationDirectory = '/tmp/assembly'
assemblyExporter.FileNames = fileToProcess


assemblyExporter.Write()

