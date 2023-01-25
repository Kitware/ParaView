import paraview.simple as smp

import PythonQt

def getPQView(view):
    model = PythonQt.paraview.pqPVApplicationCore.instance().getServerManagerModel()
    return PythonQt.paraview.pqPythonQtMethodHelpers.findProxyItem(model, view.SMProxy)

def getRenderView():
    renderView = smp.GetRenderView()
    return getPQView(renderView)

renderView = getRenderView()
PythonQt.paraview.pqRenderView.resetCamera(renderView)
