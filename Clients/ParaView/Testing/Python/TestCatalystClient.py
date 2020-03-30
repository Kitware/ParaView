from paraview.simple import *
from paraview import live

"""
This tests paraview live API to interact with Catalyst.

It does the following:
    - creates a connection
    - looks for notifications
    - extracts data and displays it locally
"""

# ----------------------------
# misc callbacks
# ----------------------------
def _updateEventCb(obj, event):
    global Updating, DataExtracted, liveInsituLink

    # Avoid recursion
    if Updating:
        return
    Updating = True

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


    if source != None:
        dataObject = source.GetClientSideObject().GetOutputDataObject(0)
        currentTime = dataObject.GetFieldData().GetArray('TimeValue').GetValue(0)
        # if connection closes due to simulation end, it creates error messages
        # leading to test failure. So we stop live visu before the end of simu.
        if  currentTime >= 0.5:
            live.PauseCatalyst(liveInsituLink)
            liveInsituLink.RemoveObservers("UpdateEvent")
            liveInsituLink.RemoveObservers("ConnectionClosedEvent")
            EndTest()
            live.PauseCatalyst(liveInsituLink)
        elif currentTime >= 0.4:
            live.PauseCatalyst(liveInsituLink, False)
            live.PauseCatalyst(liveInsituLink)
            live.PauseCatalyst(liveInsituLink)
            live.PauseCatalyst(liveInsituLink, False)

    Updating = False

def _connectionCreatedCb(obj, event):
    # name comes from the catalyst script
    name = _getSourceToExtractName(obj)
    live.ExtractCatalystData(obj, name)
    live.ProcessServerNotifications()
    Render()

def _connectionClosedCb(obj=None, event=None):
    print("Connection closed")
    Render()
    EndTest()


def EndTest():
    global renderView1
    # stop watching for events to process.
    global ProcessEvents
    ProcessEvents = False

    import os
    import sys
    try:
        baselineIndex = sys.argv.index('-B')+1
        baselinePath = sys.argv[baselineIndex]
    except:
        print("Could not get baseline directory. Test failed.")
    baseline_file = os.path.join(baselinePath, "TestCatalystClient.png")
    from paraview.vtk.test import Testing
    from paraview.vtk.util.misc import vtkGetTempDir
    Testing.VTK_TEMP_DIR = vtkGetTempDir()
    Testing.compareImage(renderView1.GetRenderWindow(), baseline_file)
    Testing.interact()


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
Updating = False

print("started")
# ----------------------------
# Create the catalyst connection
liveInsituLink = live.ConnectToCatalyst()
liveInsituLink.AddObserver("UpdateEvent", _updateEventCb)
liveInsituLink.AddObserver("ConnectionCreatedEvent", _connectionCreatedCb)
liveInsituLink.AddObserver("ConnectionClosedEvent", _connectionClosedCb)

# ----------------------------
# Automatically look for updates
_monitorServerNotifications()
