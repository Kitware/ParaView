r"""paraviewweb_wamp is the paraview-specific subclass
    of vtkweb_wamp that provides the PVWeb Application
"""

from vtk.web import wamp
from vtkParaViewWebCorePython import vtkPVWebApplication

class PVServerProtocol(wamp.ServerProtocol):
    def initApplication(self):
        return vtkPVWebApplication()
