r"""This is a module to perform live visualization in the context of Catalyst.
"""

from paraview.simple import *
from paraview import servermanager

# -----------------------------------------------------------------------------
def ConnectToCatalyst(ds_host='localhost', ds_port=22222):
    """Creates a connection to a catalyst simulation. Example usage:

    > Connect("amber") # Connect to a single server at default port
    > Connect("amber", 12345) # Connect to a single server at port 12345

    return a LiveInsituLink object.
    """
    displaySession = servermanager.ActiveConnection.Session
    insituLink = servermanager.ConnectToCatalyst(ds_host, ds_port)
    ProcessServerNotifications()
    insituLink.InvokeCommand('Initialize')
    SetActiveConnection(servermanager.GetConnectionFromSession(displaySession))
    return insituLink

# -----------------------------------------------------------------------------
def ExtractCatalystData(link, name):
    ''' Extract data called "name" from simulation managed by the LiveInsituLink link.
        Register it in displaySession
    '''
    displaySession = servermanager.ActiveConnection.Session
    proxy = link.CreateExtract('sources', name, 0)
    displaySession.GetSessionProxyManager().RegisterProxy('sources', name, proxy)
    data = FindSource(name)
    return data

# -----------------------------------------------------------------------------
def PauseCatalyst(link, pause = True):
    ''' Pause / Unpause the Catalyst simulation managed by the LiveInsituLink link.
    '''
    currentState = link.GetProperty('SimulationPaused').GetElement(0) == 1
    if currentState == pause:
        return

    link.GetProperty('SimulationPaused').SetElement(0, 1 if pause else 0)
    link.UpdateVTKObjects()
    if not pause:
        link.InvokeCommand('LiveChanged')

# -----------------------------------------------------------------------------
def ProcessServerNotifications():
    ''' Processes pending events from server.
    '''
    pm = servermanager.vtkProcessModule.GetProcessModule()
    nam = pm.GetNetworkAccessManager()
    while nam.ProcessEvents(1) == 1:
        pass
