"""
This module has utilities to benchmark paraview.

First, when run standalone, this will do a simple rendering benchmark test. This
test renders a sphere with various rendering settings and reports the rendering
rate achieved in triangles/sec. run() is the entrypoint for that usage.

Second, you can use this arbitrary pipelines and this module helps you obtain,
interpret and report about information within ParaView's logs.
Do that like so:
1) optionally, call maximize logs first
2) setup and run your visualization pipeline (via GUI or script as you prefer)
3) either
- call print_logs() to print out the logs in raw format
or
- call parse_logs() to let the script identify and report on per frame and per
filter execution times

WARNING: This was meant for server side rendering, but it could work resonably well
when geometry is delivered to the client and rendered there if the script recognize
MPIMoveData as end of frame and did something sensible on the server which has no
other end of frame knowledge

TODO: builtin mode shouldn't show server info, it is redundant
TODO: this doesn't handle split render/data server mode

TODO: the end of frame markers are heuristic, which might be buggy and generally
could use some improvement

TODO: import_logs should import saved logs from either paraview gui or the ones
print_logs produces
"""

import time
import sys
from paraview.simple import *


import re
import paraview
import copy

# a regular expression to parse filter execution time
match_filter = re.compile(" *Execute (\w+) id: +(\d+), +(\d*.*\d+) +seconds")
match_vfilter = re.compile(" *Execute (\w+) +, +(\d*.*\d+) +seconds")

# a regular expression to parse overall rendering time
match_still_render = re.compile(" *(Still) Render, +(\d*.*\d+) +seconds")
match_interactive_render = re.compile(" *(Interactive) Render, +(\d*.*\d+) +seconds")
match_render = re.compile(" *(\w+|\w+ Dev) Render, +(\d*.*\d+) +seconds")
match_icetrender = re.compile("(IceT Dev) Render, +(\d*.*\d+) +seconds")

# more for parallel composite and delivery time
match_composite = re.compile(" *Compositing, +(\d*.*\d+) +seconds")
match_send = re.compile(" *Sending, +(\d*.*\d+) +seconds")
match_receive = re.compile(" *Receiving, +(\d*.*\d+) +seconds")

match_comp_xmit = re.compile(" *TreeComp (Send|Receive) (\d+) (to|from) (\d+) uchar (\d+), +(\d*.*\d+) +seconds")
match_comp_comp = re.compile(" *TreeComp composite, *(\d*.*\d+) +seconds")

showparse = False

#icet composite message comes after the render messages, where for bswap and manta
#it comes before so we have to treat icet differently
icetquirk = False

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

logs = []

def maximize_logs () :
    """
    Convenience method to ask paraview to produce logs with lots of space and
    highest resolution.
    """
    pm = paraview.servermanager.vtkProcessModule.GetProcessModule()
    if pm == None:
        return

    acid = servermanager.ActiveConnection.ID #:) acronym
    pm.SetLogBufferLength(acid, 0x1, 1000000)
    pm.SetLogBufferLength(acid, 0x4, 1000000)
    pm.SetLogBufferLength(acid, 0x10, 1000000)
    pm.SetLogThreshold(acid, 0x1, 0.0)
    pm.SetLogThreshold(acid, 0x4, 0.0)
    pm.SetLogThreshold(acid, 0x10, 0.0)


def import_logs( filename ) :
    """
    This is for bringing in a saved log files and parse it after the fact.
    """
    #TODO: Fill this in
    global logs
    logs = []

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

    pmOptions = pm.GetOptions()
    """
    vtkPVOptions::ProcessTypeEnum
    PARAVIEW = 0x2,
    PVCLIENT = 0x4,
    PVSERVER = 0x8,
    PVRENDER_SERVER = 0x10,
    PVDATA_SERVER = 0x20,
    PVBATCH = 0x40,
    """
    if pmOptions.GetProcessType() == 0x40:
        runmode = 'batch'
    else:
        runmode = 'interactive'

    if not pm.GetRenderClientMode(connectionId):
        servertype = 'unified'
    else:
        servertype = 'split'

    """
    vtkProcessModule::SERVER_FLAGS
    DATA_SERVER = 0x01,
    DATA_SERVER_ROOT = 0x02,
    RENDER_SERVER = 0x04,
    RENDER_SERVER_ROOT = 0x08,
    SERVERS = DATA_SERVER | RENDER_SERVER,
    CLIENT = 0x10,
    CLIENT_AND_SERVERS = DATA_SERVER | CLIENT | RENDER_SERVER
    """
    if runmode == 'batch':
        components = [0x04]
    else:
        if servertype == 'unified':
            components = [0x10, 0x04]
        else:
            components = [0x10, 0x04, 0x01]

    for component in components:
        timerInfo = paraview.servermanager.vtkPVTimerInformation()
        pm.GatherInformation(connectionId, component,\
                             timerInfo, pm.GetProcessModuleIDAsInt())

        for i in range(timerInfo.GetNumberOfLogs()):
            alog = OneLog()
            alog.runmode = runmode
            alog.servertype = servertype
            alog.component = component
            alog.rank = i
            for line in timerInfo.GetLog(i).split('\n'):
                alog.lines.append(line)
            logs.append(alog)

def print_logs() :
    global logs

    if len(logs) == 0:
        get_logs()

    for i in logs:
       i.print_log(True)

def __process_frame() :
    global filters
    global current_frames_records
    global frames

    max = len(current_frames_records)

    #for x in xrange(0,max):
    #    print current_frames_records[x]

    #determine ancestry of each record from order and indent
    #subtract only immediate children from each record

    #TODO: Make this an option
    for x in xrange(0,max):
        indent = current_frames_records[x]['indent']
        minindent = 10000
        for y in xrange(x+1,max):
            indent2 = current_frames_records[y]['indent']
            if indent2<=indent:
                #found a record which is not a decendent
                break
            if indent2 < minindent:
                minindent = indent2
        for y in xrange(x+1,max):
            indent2 = current_frames_records[y]['indent']
            if indent2 == minindent:
                current_frames_records[x]['local_duration'] = \
                current_frames_records[x]['local_duration'] -\
                current_frames_records[y]['duration']

    #for x in xrange(0,max):
    #    print current_frames_records[x]

    for x in xrange(0,max):
        #keep global statics per filter
        record = current_frames_records[x]
        id = record['id']
        if id in filters:
            srecord = filters[id]
            srecord['duration'] = srecord['duration'] + record['duration']
            srecord['local_duration'] = srecord['local_duration'] + record['local_duration']
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

def parse_logs(show_parse = False) :
    """
    Parse the collected paraview log information.
    This prints out per frame, and aggregated per filter statistics.

    If show_parse is true, debugging information is shown about the parsing process
    that allows you to verify that the derived stats are correct.
    This includes each and echo of each log line collected, prepended by
    the token type and indent scanned in, or ???? if the line is unrecognized
    and ignored. Frame boundaries are denoted by SOF, indicating the preceeding line
    was determined to be the start of the next frame.
    """

    global filters
    global current_frames_records
    global frames
    global cnt
    global showparse

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
        print "#FRAME TIMINGS"
        print "#filter id, filter type, inclusive duration, local duration"
        for cnt in xrange(len(frames)):
            print "#Frame ", cnt
            for record in frames[cnt]:
                if 'id' in record:
                    print record['id'], ",",
                    print record['name'], ",",
                    print record['duration'], ",",
                    print record['local_duration']
        print
        print

        #print out the gathered per filter information
        print "#FILTER TIMINGS"
        print "#filter id, filter type, count, sum inclusive duration, sum local duration"
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
    v.UseImmediateMode = 0
    v.UseTriangleStrips = 0

    # Test different configurations
    v.UseImmediateMode = 0
    title = 'display lists, no triangle strips, solid color'
    v.UseTriangleStrips = 0
    results.append(__render(ss, v, title, nframes))

    title = 'display lists, triangle strips, solid color'
    v.UseTriangleStrips = 1
    results.append(__render(ss, v, title, nframes))

    v.UseImmediateMode = 1
    title = 'no display lists, no triangle strips, solid color'
    v.UseTriangleStrips = 0
    results.append(__render(ss, v, title, nframes))

    title = 'no display lists, triangle strips, solid color'
    v.UseTriangleStrips = 1
    results.append(__render(ss, v, title, nframes))

    # Color by normals
    lt = servermanager.rendering.PVLookupTable()
    rep.LookupTable = lt
    rep.ColorAttributeType = 0 # point data
    rep.ColorArrayName = "Normals"
    lt.RGBPoints = [-1, 0, 0, 1, 0.0288, 1, 0, 0]
    lt.ColorSpace = 'HSV'
    lt.VectorComponent = 0

    v.UseImmediateMode = 0
    title = 'display lists, no triangle strips, color by array'
    v.UseTriangleStrips = 0
    results.append(__render(ss, v, title, nframes))

    title = 'display lists, triangle strips, color by array'
    v.UseTriangleStrips = 1
    results.append(__render(ss, v, title, nframes))
    v.UseImmediateMode = 1

    v.UseImmediateMode = 1
    title = 'no display lists, no triangle strips, color by array'
    v.UseTriangleStrips = 0
    results.append(__render(ss, v, title, nframes))

    title = 'no display lists, triangle strips, color by array'
    v.UseTriangleStrips = 1
    results.append(__render(ss, v, title, nframes))

    if filename:
        f = open(filename, "w")
    else:
        f = sys.stdout
    print >>f, 'configuration, %d, %d' % (results[0][1][0], results[0][2][0])
    for i in results:
        print >>f, '"%s", %g, %g' % (i[0], i[1][1], i[2][1])

if __name__ == "__main__":
    run()
