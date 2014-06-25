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

# a regular expression to parse filter execution time
match_filter = re.compile(" *Execute (\w+) id: +(\d+), +(\d*.*\d+) +seconds")
match_vfilter = re.compile(" *Execute (\w+) +, +(\d*.*\d+) +seconds")

# a regular expression to parse overall rendering time
match_still_render = re.compile(" *(Still) Render, +(\d*.*\d+) +seconds")
match_interactive_render = \
re.compile(" *(Interactive) Render, +(\d*.*\d+) +seconds")
match_render = re.compile(" *(\w+|\w+ Dev) Render, +(\d*.*\d+) +seconds")
match_icetrender = re.compile("(IceT Dev) Render, +(\d*.*\d+) +seconds")

# more for parallel composite and delivery time
match_composite = re.compile(" *Compositing, +(\d*.*\d+) +seconds")
match_send = re.compile(" *Sending, +(\d*.*\d+) +seconds")
match_receive = re.compile(" *Receiving, +(\d*.*\d+) +seconds")

match_comp_xmit = \
re.compile(" *TreeComp (Send|Receive) (\d+) " + \
           "(to|from) (\d+) uchar (\d+), +(\d*.*\d+) +seconds")
match_comp_comp = re.compile(" *TreeComp composite, *(\d*.*\d+) +seconds")

showparse = False

#icet composite message comes after the render messages,
#where for bswap and manta it comes before so we have to treat icet differently
icetquirk = False

start_frame = 0
default_log_threshold = dict()

class OneLog :
    def __init__(self):
        self.runmode = 'batch'
        self.servertype = 'unified'
        self.component = 0x10
        self.rank = 0
        self.lines = []

    def componentString(self):
        ret = ""
        if self.component & 0x10:
            ret = ret + " CLIENT "
        if self.component & 0x4:
            ret = ret + " RENDER "
        if self.component & 0x1:
            ret = ret + " DATA "
        return ret

    def print_log(self, showlines=False):
        print "#RunMode:", self.runmode,
        print "ServerType:", self.servertype,
        print "Component:", self.componentString(),
        print "processor#:", self.rank
        if showlines:
            for i in self.lines:
                print i

    def toString(self, showlines=False):
        result = "#RunMode: " + self.runmode + " ServerType: " + self.servertype + " Component: " + self.componentString() + " processor#: " + str(self.rank) + "\n"
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
            alog.component = component
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

def __process_frame() :
    global filters
    global current_frames_records
    global frames
    global start_frame

    max = len(current_frames_records)

    #determine ancestry of each record from order and indent
    #subtract only immediate children from each record

    #TODO: Make this an option
    for x in xrange(max):
        indent = current_frames_records[x]['indent']
        minindent = 10000
        for y in xrange(x+1,max):
            indent2 = current_frames_records[y]['indent']
            if indent2<=indent:
                #found a record which is not a descendant
                break
            if indent2 < minindent:
                minindent = indent2
        for y in xrange(x+1,max):
            indent2 = current_frames_records[y]['indent']
            if indent2 == minindent:
                current_frames_records[x]['local_duration'] = \
                current_frames_records[x]['local_duration'] -\
                current_frames_records[y]['duration']

    for x in xrange(max):
        #keep global statics per filter
        record = current_frames_records[x]
        id = record['id']
        if id in filters:
            srecord = filters[id]
            srecord['duration'] = srecord['duration'] + record['duration']
            srecord['local_duration'] = srecord['local_duration'] +\
                                        record['local_duration']
            srecord['count'] = srecord['count'] + 1
            filters[id] = srecord
        else:
            filters[id] = copy.deepcopy(record)

    #save off this frame and begin the next
    frames.append(current_frames_records)
    current_frames_records = []

def __parse_line (line) :
    """
    Examine one line from the logs. If it is a report about a filter's
    execution time, parse the relevant information out of the line and
    collect those statistics. We record each filter's average execution
    time as well as the each filters contribution to the each rendered frame.
    """
    global filters
    global current_frames_records
    global cnt
    global show_input
    global icetquirk

    found = False

    #find indent
    cnt = 0
    for c in range(len(line)):
        if line[c] == " ":
            cnt = cnt + 1
        else:
            break

    #determine if this log comes from icet so we can
    #do special case treatement for frame markings
    icetline = False
    match = match_icetrender.match(line)
    if match != None:
        icetquirk = True
        icetline = True

    match = match_filter.match(line)
    if match != None:
        found = True
        if showparse:
            print "FILT:", cnt, line
        name = match.group(1)
        id = match.group(2)
        duration = match.group(3)

    match = match_vfilter.match(line)
    if match != None:
        found = True
        if showparse:
            print "LFLT:", cnt, line
        name = match.group(1)
        id = name
        duration = match.group(2)

    match = match_comp_comp.match(line)
    if match != None:
        found = True
        if showparse:
            print "TCMP:", cnt, line
        name = "tree comp"
        id = name
        duration = match.group(1)

    match = match_comp_xmit.match(line)
    if match != None:
        found = True
        if showparse:
            print "TXMT:", cnt, line
        name = match.group(1)
        id = name
        duration = match.group(6)

    match = match_composite.match(line)
    if match != None:
        found = True
        if showparse:
            print "COMP:", cnt, line
        name = 'composite'
        id = 'comp'
        duration = match.group(1)

    match = match_send.match(line)
    if match != None:
        found = True
        if showparse:
            print "SEND:", cnt, line
        name = 'send'
        id = 'send'
        duration = match.group(1)

    match = match_receive.match(line)
    if match != None:
        found = True
        if showparse:
            print "RECV:", cnt, line
        name = 'receive'
        id = 'recv'
        duration = match.group(1)

    match = match_still_render.match(line)
    if match != None:
        found = True
        if showparse:
            print "STILL:", cnt, line
        name = match.group(1)
        id = 'still'
        duration = match.group(2)

    if match == None:
        match = match_interactive_render.match(line)
        if match != None:
            found = True
            if showparse:
                print "INTER:", cnt, line
            name = match.group(1)
            id = 'inter'
            duration = match.group(2)

    if match == None:
        match = match_render.match(line)
        if match != None:
            found = True
            if showparse:
                print "REND:", cnt, line
            name = match.group(1)
            id = 'render'
            duration = match.group(2)

    if found == False:
        # we didn't find anything we recognized in this line, ignore it
        if showparse:
            print "????:", cnt, line
        return

    record = dict()
    record['id'] = id
    record['name'] = name
    record['duration'] = float(duration)
    record['local_duration'] = float(duration)
    record['count'] = 1
    record['indent'] = cnt

    #watch for the beginning of the next frame/end of previous frame
    if cnt == 0:
        if (id == 'still') or \
           (id == 'inter') or \
           (icetquirk == False and id == 'comp') or \
           (icetquirk == True and icetline == True) :
            if showparse:
                print "SOF" #start of frame
            #decipher parent child information from records in the frame
            #and save off newly gathered per filter and per frame statistics
            __process_frame()

    #keep a record of this execution as part for the current frame
    current_frames_records.append(record)

    return

def parse_logs(show_parse = False, tabular = False) :
    """
    Parse the collected paraview log information.
    This prints out per frame, and aggregated per filter statistics.

    If show_parse is true, debugging information is shown about the parsing
    process that allows you to verify that the derived stats are correct.
    This includes each and echo of each log line collected, prepended by
    the token type and indent scanned in, or ???? if the line is unrecognized
    and ignored. Frame boundaries are denoted by SOF, indicating the preceeding
    line was determined to be the start of the next frame.
    """

    global filters
    global current_frames_records
    global frames
    global cnt
    global showparse
    global start_frame

    showparse = show_parse

    if len(logs) == 0:
        get_logs()

    for i in logs:
        # per filter records
        filters = dict()
        filters.clear()
        # per frame records
        frames = []
        # components of current frame
        current_frames_records = []
        cnt = 0

        runmode = i.runmode
        servertype = i.servertype
        component = i.component
        rank = i.rank
        i.print_log(False)

        for line in i.lines:
            __parse_line(line)

        #collect stats for the current frame in process but not officially ended
        __process_frame()

        #print out the gathered per frame information
        if tabular:
            frecs = dict()
            line = "#framenum, "
            for x in filters:
                line += filters[x]['name'] + ":" + filters[x]['id']  + ", "
            #print line
            for cnt in xrange(start_frame, len(frames)):
                line = ""
                line += str(cnt) + ", "
                printed = dict()
                for x in filters:
                    id = filters[x]['id']
                    name = filters[x]['name']
                    found = False
                    for record in frames[cnt]:
                        if 'id' in record:
                            if record['id'] == id and \
                            record['name'] == name and \
                            not id in printed:
                                found = True
                                printed[id] = 1
                                line += str(record['local_duration']) + ", "
                                if not id in frecs:
                                    frecs[id] = []
                                frecs[id].append(record['local_duration'])
                    if not found:
                        line += "0, "
                #print line
            #print
            for x in frecs.keys():
                v = frecs[x]
                print "# ", x, len(v),
                if numpy_loaded:
                    print numpy.min(v), numpy.mean(v), numpy.max(v),
                    print numpy.std(v)
        else:
            print "#FRAME TIMINGS"
            print "#filter id, filter type, inclusive duration, local duration"
            for cnt in xrange(start_frame, len(frames)):
                print "#Frame ", cnt
                for record in frames[cnt]:
                    if 'id' in record:
                        print record['id'], ",",
                        print record['name'], ",",
                        print record['duration'], ",",
                        print record['local_duration']
        #print
        #print

        if not tabular:
            #print out the gathered per filter information
            print "#FILTER TIMINGS"
            print "#filter id, filter type, count, "+\
                  "sum inclusive duration, sum local duration"
            for x in filters:
                record = filters[x]
                print record['id'], ",",
                print record['name'], ",",
                print record['count'], ",",
                print record['duration'], ",",
                print record['local_duration']
            print

def __render(ss, v, title, nframes):
    print '============================================================'
    print title
    res = []
    res.append(title)
    for phires in (500, 1000):
        ss.PhiResolution = phires
        c = v.GetActiveCamera()
        v.CameraPosition = [-3, 0, 0]
        v.CameraFocalPoint = [0, 0, 0]
        v.CameraViewUp = [0, 0, 1]
        Render()
        c1 = time.time()
        for i in range(nframes):
            c.Elevation(0.5)
            Render()
            if not servermanager.fromGUI:
                sys.stdout.write(".")
                sys.stdout.flush()
        if not servermanager.fromGUI:
            sys.stdout.write("\n")
        tpr = (time.time() - c1)/nframes
        ncells = ss.GetDataInformation().GetNumberOfCells()
        print tpr, " secs/frame"
        print ncells, " polys"
        print ncells/tpr, " polys/sec"

        res.append((ncells, ncells/tpr))
    return res

def run(filename=None, nframes=60):
    """ Runs the benchmark. If a filename is specified, it will write the
    results to that file as csv. The number of frames controls how many times
    a particular configuration is rendered. Higher numbers lead to more accurate
    averages. """
    # Turn off progress printing
    paraview.servermanager.SetProgressPrintingEnabled(0)

    # Create a sphere source to use in the benchmarks
    ss = Sphere(ThetaResolution=1000, PhiResolution=500)
    rep = Show()
    v = Render()
    results = []

    # Start with these defaults
    #v.RemoteRenderThreshold = 0
    obj = servermanager.misc.GlobalMapperProperties()
    obj.GlobalImmediateModeRendering = 0

    # Test different configurations
    title = 'display lists, no triangle strips, solid color'
    obj.GlobalImmediateModeRendering = 0
    results.append(__render(ss, v, title, nframes))

    title = 'no display lists, no triangle strips, solid color'
    obj.GlobalImmediateModeRendering = 1
    results.append(__render(ss, v, title, nframes))

    # Color by normals
    lt = servermanager.rendering.PVLookupTable()
    rep.LookupTable = lt
    rep.ColorAttributeType = 0 # point data
    rep.ColorArrayName = "Normals"
    lt.RGBPoints = [-1, 0, 0, 1, 0.0288, 1, 0, 0]
    lt.ColorSpace = 'HSV'
    lt.VectorComponent = 0

    title = 'display lists, no triangle strips, color by array'
    obj.GlobalImmediateModeRendering = 0
    results.append(__render(ss, v, title, nframes))

    title = 'no display lists, no triangle strips, color by array'
    obj.GlobalImmediateModeRendering = 1
    results.append(__render(ss, v, title, nframes))

    if filename:
        f = open(filename, "w")
    else:
        f = sys.stdout
    print >>f, 'configuration, %d, %d' % (results[0][1][0], results[0][2][0])
    for i in results:
        print >>f, '"%s", %g, %g' % (i[0], i[1][1], i[2][1])


def test_module():
    """Simply exercises a few components of the module."""
    maximize_logs()

    paraview.servermanager.SetProgressPrintingEnabled(0)
    ss = Sphere(ThetaResolution=1000, PhiResolution=500)
    rep = Show()
    v = Render()

    print_logs()

if __name__ == "__main__":
    if "--test" in sys.argv:
        test_module()
    else:
        run()
