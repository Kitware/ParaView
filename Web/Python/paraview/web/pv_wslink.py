r"""paraviewweb_wslink is the paraview-specific subclass
    of vtkweb_wslink that provides the PVWeb Application
"""

from vtkmodules.web import wslink as vtk_wslink
from paraview.modules.vtkPVClientWeb import vtkPVWebApplication
from paraview.web import protocols as pv_protocols

class PVServerProtocol(vtk_wslink.ServerProtocol):

    def __init__(self):
        vtk_wslink.ServerProtocol.__init__(self)
        # if (vtk_wslink.imageCapture):
        #   self.unregisterLinkProtocol(vtk_wslink.imageCapture)
        # vtk_wslink.imageCapture = pv_protocols.ParaViewWebViewPortImageDelivery()
        # self.registerLinkProtocol(vtk_wslink.imageCapture)
        # vtk_wslink.imageCapture.setApplication(self.getApplication())

    def initApplication(self):
        return vtkPVWebApplication()
