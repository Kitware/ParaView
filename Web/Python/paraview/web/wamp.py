r"""paraviewweb_wamp is the paraview-specific subclass
    of vtkweb_wamp that provides the PVWeb Application
"""

from vtk.web import wamp
from vtk.vtkParaViewWebCore import vtkPVWebApplication

from paraview.web import protocols as pv_protocols

class PVServerProtocol(wamp.ServerProtocol):

    def __init__(self, config):
        wamp.ServerProtocol.__init__(self, config)
        wamp.imageCapture = pv_protocols.ParaViewWebViewPortImageDelivery()
        wamp.imageCapture.setApplication(self.Application)

    def initApplication(self):
        return vtkPVWebApplication()
