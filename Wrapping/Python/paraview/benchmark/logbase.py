"""
This module has utilities to benchmark paraview.

You can set up arbitrary pipelines and this module helps you obtain,
interpret and report the information recorded by ParaView's logs.
Do that like so:

1. Optionally, call maximize logs first
2. Setup and run your visualization pipeline (via GUI or script as you prefer)
3. Call print_logs() to print out the logs in raw format
"""

from __future__ import print_function
from paraview.simple import *
import paraview

start_frame = 0
default_log_threshold = dict()

def component_to_string(comp):
    session = servermanager.ProxyManager().GetSessionProxyManager().GetSession()
    if comp == session.NONE: return 'None'
    if comp == session.DATA_SERVER: return 'DataServer'
    if comp == session.DATA_SERVER_ROOT: return 'DataServerRoot'
    if comp == session.RENDER_SERVER: return 'RenderServer'
    if comp == session.RENDER_SERVER_ROOT: return 'RenderServerRoot'
    if comp == session.SERVERS: return 'Servers'
    if comp == session.CLIENT: return 'Client'
    if comp == session.CLIENT_AND_SERVERS: return 'ClientAndServers'
    return None

class OneLog :
    def __init__(self, runmode='batch', servertype='unified', component=0, rank=0):
        self.runmode = runmode
        self.servertype = servertype
        self.component = component_to_string(component)
        self.rank = rank
        self.lines = []

    def print_log(self, showlines=False):
        print ("#RunMode:", self.runmode, end="")
        print ("ServerType:", self.servertype, end="")
        print ("Component:", self.component, end="")
        print ("processor#:", self.rank)
        if showlines:
            for i in self.lines:
                print (i)

    def toString(self, showlines=False):
        result = "#RunMode: " + self.runmode + " ServerType: " + self.servertype + " processor#: " + str(self.rank) + "\n"
        if showlines:
            for i in self.lines:
                result += i + "\n"
        return result

logs = []

def maximize_logs () :
    """
    Convenience method to ask paraview to produce logs with lots of space and
    highest resolution.
    """
    pm = paraview.servermanager.vtkProcessModule.GetProcessModule()
    if pm == None:
        return

    ss = paraview.servermanager.vtkSMSession
    for ptype in [ss.CLIENT_AND_SERVERS, ss.CLIENT, ss.SERVERS,
                 ss.RENDER_SERVER, ss.DATA_SERVER]:
      default_log_threshold[str(ptype)] = 0.0

    pxm = paraview.servermanager.ProxyManager()
    tl = pxm.NewProxy("misc", "TimerLog")
    prop = tl.GetProperty("MaxEntries")
    prop.SetElements1(1000000)
    tl.UpdateVTKObjects()

def get_memuse() :
    pm = paraview.servermanager.vtkProcessModule.GetProcessModule()
    session = servermanager.ProxyManager().GetSessionProxyManager().GetSession()

    retval = []
    if pm.GetProcessTypeAsInt() == pm.PROCESS_BATCH:
        components = {'CL_DS_RS': session.CLIENT_AND_SERVERS}
    else:
        if session.GetRenderClientMode() == session.RENDERING_UNIFIED:
            components = {'CL': session.CLIENT, 'DS_RS': session.SERVERS}
        else:
            components = {'CL': session.CLIENT, 'RS': session.RENDER_SERVER, 'DS': session.DATA_SERVER}

    for comp_label, comp_type in components.items():
        infos = servermanager.vtkPVMemoryUseInformation()
        session.GatherInformation(comp_type, infos, 0)
        for i in range(0,infos.GetSize()):
            retval.append('%(l)s[%(r)d] %(pu)d / %(hu)d' %
                          {'l': comp_label, 'r': infos.GetRank(i),
                           'pu': infos.GetProcMemoryUse(i),
                           'hu': infos.GetHostMemoryUse(i)})
    return retval

def dump_logs( filename ) :
    """
    This saves off the logs we've gathered.
    Ot allows you to run a benchmark somewhere, save off all of the details in
    raw format, then load them somewhere else. You can then do a detailed
    analysis and you always have the raw data to go back to.
    """
    import pickle
    global logs
    if sys.version_info < (3,):
        f = open(filename, "w")
    else:
        f = open(filename, "wb")
    pickle.dump(logs, f)
    f.close()

def import_logs( filename ) :
    """
    This is for bringing in a saved log files and parse it after the fact.
    TODO: add an option to load in raw paraview logs in text format
    """
    import pickle
    global logs
    logs = []
    if sys.version_info < (3,):
        f = open(filename, "r")
    else:
        f = open(filename, "rb")
    logs = pickle.load(f)
    f.close()

def get_logs() :
    """
    This is for bringing in logs at run time to parse while running.
    """
    global logs
    logs = []

    pm = paraview.servermanager.vtkProcessModule.GetProcessModule()
    if pm == None:
        return

    connectionId = paraview.servermanager.ActiveConnection.ID
    session = paraview.servermanager.ActiveConnection.Session

    is_symmetric_mode = False
    if pm.GetProcessTypeAsInt() == pm.PROCESS_BATCH:
        runmode = 'batch'
        is_symmetric_mode = pm.GetSymmetricMPIMode()
    else:
        runmode = 'interactive'

    if session.GetRenderClientMode() == session.RENDERING_UNIFIED:
        servertype = 'unified'
    else:
        servertype = 'split'

    if runmode == 'batch':
        # collect information from all processes in one go.
        components = [session.CLIENT_AND_SERVERS]
    else:
        if servertype == 'unified':
            # collect information separately for client and servers.
            components = [session.CLIENT, session.SERVERS]
        else:
            # collect information separately for all process types.
            components = [session.CLIENT, session.RENDER_SERVER, session.DATA_SERVER]

    for component in components:
        timerInfo = paraview.servermanager.vtkPVTimerInformation()
        if len(default_log_threshold) != 0:
           timerInfo.SetLogThreshold(default_log_threshold[str(component)])
        session.GatherInformation(component, timerInfo, 0)

        for i in range(timerInfo.GetNumberOfLogs()):
            alog = OneLog(runmode, servertype, component, i)

            if is_symmetric_mode:
                # in Symmetric mode, GatherInformation() only collects
                # information from the current node. so the
                # vtkPVTimerInformation will only have info for local process.
                alog.rank = pm.GetPartitionId()

            alog.lines = timerInfo.GetLog(i).split('\n');
            logs.append(alog)

def print_logs() :
    """
    Print logs on the root node by gathering logs across all the nodes
    regardless if the process was started in symmetric mode or not.
    """
    global logs

    if len(logs) == 0:
        get_logs()

    # Handle symmetric mode specifically if need be
    pm = paraview.servermanager.vtkProcessModule.GetProcessModule()
    is_symmetric_mode = False
    if pm != None:
        is_symmetric_mode = pm.GetSymmetricMPIMode()

    if is_symmetric_mode:
        # Need to provide extra synchronization
        ctrl = pm.GetGlobalController()
        proc = pm.GetPartitionId()
        nbProc = pm.GetNumberOfLocalPartitions()
        if proc == 0:
            # Start with my logs
            for i in logs:
                i.print_log(True)
            # Then Print the log of every other rank
            for otherProc in range(1, nbProc):
                # Max buffer size 999999
                logSize = " " * 6
                ctrl.Receive(logSize, len(logSize), otherProc, 987455)
                logSize = int(logSize)
                logTxt = " " * logSize
                ctrl.Receive(logTxt, logSize, otherProc, 987456)
                print (logTxt)
        else:
            # Extract logs text
            logTxt = ""
            for i in logs:
                logTxt += i.toString(True)
            logSize = str(len(logTxt))

            # Push local logs to process 0
            ctrl.Send(logSize, len(logSize), 0, 987455)
            ctrl.Send(logTxt, len(logTxt), 0, 987456)

    else:
        # Regular local print
        for i in logs:
            i.print_log(True)


def test_module():
    """Simply exercises a few components of the module."""
    maximize_logs()

    paraview.servermanager.SetProgressPrintingEnabled(0)
    ss = Sphere(ThetaResolution=1000, PhiResolution=500)
    rep = Show()
    v = Render()

    print_logs()

if __name__ == "__main__":
    test_module()
