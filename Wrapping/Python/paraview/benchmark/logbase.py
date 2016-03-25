"""
This module has utilities to benchmark paraview.

First, when run standalone, this will do a simple rendering benchmark test. The
test renders a sphere with various rendering settings and reports the rendering
rate achieved in triangles/sec. run() is the entrypoint for that usage.

Second, you can set up arbitrary pipelines and this module helps you obtain,
interpret and report the information recorded by ParaView's logs.
Do that like so:

1. optionally, call maximize logs first
2. setup and run your visualization pipeline (via GUI or script as you prefer)
3. either
      call print_logs() to print out the logs in raw format

      call parse_logs() to let the script identify and report on per frame and per filter execution times


::

    WARNING: This was meant for server side rendering, but it could work
             reasonably well when geometry is delivered to the client and rendered there
             if the script were changed to recognize MPIMoveData as end of frame and did
             something sensible on the server which has no other end of frame knowledge

    TODO: builtin mode shouldn't show server info, it is redundant
    TODO: this doesn't handle split render/data server mode
    TODO: the end of frame markers are heuristic, likely buggy, and have not
          been tried since before 3.9's view restructuring
"""

import time
import sys
from paraview.simple import *

try:
    import numpy
    numpy_loaded = True
except ImportError:
    numpy_loaded = False

import re
import paraview
import copy
import pickle

start_frame = 0
default_log_threshold = dict()

class OneLog :
    def __init__(self):
        self.runmode = 'batch'
        self.servertype = 'unified'
        self.rank = 0
        self.lines = []

    def print_log(self, showlines=False):
        print "#RunMode:", self.runmode,
        print "ServerType:", self.servertype,
        print "processor#:", self.rank
        if showlines:
            for i in self.lines:
                print i

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
    session = servermanager.ProxyManager().GetSessionProxyManager().GetSession()

    retval = []
    infos = servermanager.vtkPVMemoryUseInformation()
    session.GatherInformation(session.CLIENT, infos, 0)
    procUse = str(infos.GetProcMemoryUse(0))
    hostUse = str(infos.GetHostMemoryUse(0))
    retval.append("CLIENT " + procUse + " / " + hostUse)

    infos = servermanager.vtkPVMemoryUseInformation()
    session.GatherInformation(session.DATA_SERVER, infos, 0)
    for i in range(0,infos.GetSize()):
        rank = str(infos.GetRank(i))
        procUse = str(infos.GetProcMemoryUse(i))
        hostUse = str(infos.GetHostMemoryUse(i))
        retval.append("DS[" + rank + "] " + procUse + " / " + hostUse)
    return retval

def dump_logs( filename ) :
    """
    This saves off the logs we've gathered.
    Ot allows you to run a benchmark somewhere, save off all of the details in
    raw format, then load them somewhere else. You can then do a detailed
    analysis and you always have the raw data to go back to.
    """
    global logs
    f = open(filename, "w")
    pickle.dump(logs, f)
    f.close()

def import_logs( filename ) :
    """
    This is for bringing in a saved log files and parse it after the fact.
    TODO: add an option to load in raw parview logs in text format
    """
    global logs
    logs = []
    f = open(filename, "r")
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
            alog = OneLog()
            alog.runmode = runmode
            alog.servertype = servertype
            alog.rank = i

            if is_symmetric_mode:
                # in Symmetric mode, GatherInformation() only collects
                # information from the current node. so the
                # vtkPVTimerInformation will only have info for local process.
                alog.rank = pm.GetPartitionId()

            for line in timerInfo.GetLog(i).split('\n'):
                alog.lines.append(line)
            logs.append(alog)

def print_logs() :
    """
    Print logs on the root node by gathering logs accross all the nodes
    regardless if the process was started in symmetric mode or not.
    """
    global logs

    if len(logs) == 0:
        get_logs()

    # Handle symetric mode specificaly if need be
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
                print logTxt
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
