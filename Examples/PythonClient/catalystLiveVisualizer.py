from paraview.simple import *
from paraview import live

"""
This example demonstrates how to use python to connect to a Catalyst session.

At each timestep, the slice is extracted and a non-interactive render view
is updated. When the connection is closed (due to simulation end),
the interaction is enabled in the view.

First make sure that LiveVisualization is enabled in the script. Then run this example.
Last run the simulation.

For instance:
    modify Examples/Catalyst/CxxFullExample/SampleScripts/feslicescript.py
    ```
    coprocessor.EnableLiveVisualization(True)
    ```
    Then run:
    ```
    $ pvpython catalystLiveVisualizer.py
    $ ./CxxFullExample feslicescript.py
    ```
"""

# ----------------------------
# misc callbacks
# ----------------------------
def _updateEventCb(obj, event):
    global DataExtracted

    name = _getSourceToExtractName(obj)
    source = FindSource(name)

    if source != None and not DataExtracted:
        DataExtracted = True
        rep = Show(source)
        rep.Representation = 'Surface'
        info = source.GetPointDataInformation()
        if info.GetNumberOfArrays() > 0:
            arrName = info.GetArray(0).Name
            rep.ColorArrayName = ['POINTS', arrName]
            ColorBy(rep, arrName)

    prop = GetDisplayProperties(source)
    prop.RescaleTransferFunctionToDataRange(False, True)
    ResetCamera()
    Render()

def _connectionCreatedCb(obj, event):
    # name comes from the catalyst script
    name = _getSourceToExtractName(obj)
    live.ExtractCatalystData(obj, name)
    live.ProcessServerNotifications()
    Render()

def _connectionClosedCb(obj, event):
    global ProcessEvents
    ProcessEvents = False
    print("Connection closed, interaction enabled.\nPress 'q' to quit.")
    Interact()
    print("Exit.")


# ----------------------------
# Utilities methods
# ----------------------------
def _monitorServerNotifications():
    import threading
    global ProcessEvents
    ProcessEvents = True
    live.ProcessServerNotifications()
    if ProcessEvents:
        threading.Timer(.2, _monitorServerNotifications).start()

def _getSourceToExtractName(insituLink):
    pm = insituLink.GetInsituProxyManager()
    name = 'input'
    proxy = pm.GetProxy('sources', name)
    if proxy == None:
        # if no one exists, pick another one
        name = pm.GetProxyName('sources', pm.GetNumberOfProxies('sources') - 1)

    return name
# ----------------------------
# Create a render view
renderView1 = CreateRenderView()

DataExtracted = False

# ----------------------------
# Create the catalyst connection
liveInsituLink = live.ConnectToCatalyst()
liveInsituLink.AddObserver("UpdateEvent", _updateEventCb)
liveInsituLink.AddObserver("ConnectionCreatedEvent", _connectionCreatedCb)
liveInsituLink.AddObserver("ConnectionClosedEvent", _connectionClosedCb)

# ----------------------------
# Automatically look for updates
_monitorServerNotifications()
