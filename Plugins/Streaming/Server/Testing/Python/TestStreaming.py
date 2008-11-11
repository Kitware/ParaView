## TestStreaming.py
## Test Streaming Paraview
##
## Usage: pvbatch testStreaming.py --plugin <plugin-filename> [OPTIONS]
##
#########################################################################


import sys
from paraview import servermanager

###########################################################################
## Parse arguments
from optparse import OptionParser
parser = OptionParser()
parser.add_option("-q", "--quiet",
                  action="store_false", dest="verbose", default=True,
                  help="don't print progress to stdout")
parser.add_option("-l", "--log",
                  action="store_true", dest="log", default=False,
                  help="print timing log to stdout")
parser.add_option("-d", "--debug",
                  action="store_true", dest="debug", default=False,
                  help="print streaming debug messages")
parser.add_option("-p", "--passes", dest="passes", default=16, type="int",
                  help="set number of streaming passes to PASSES (default is 16)", metavar="PASSES")
parser.add_option("-c", "--cachesize", dest="cachesize", default=16, type="int",
                  help="set piece cache filter's cache size to SIZE (default is 16)", metavar="SIZE")
parser.add_option("-w", "--wait", dest="wait", default=0, type="int",
                  help="wait SECONDS after the final render before closing the window", metavar="SECONDS")
parser.add_option("--plugin", dest="plugin", default="",
                  help="path to streaming plugin")

(options, args) = parser.parse_args()

printProgress = options.verbose
streamMessages = options.debug
numberOfPasses = options.passes
cacheSize = options.cachesize
printLog = options.log
sleepTime = options.wait
streamingPlugin = options.plugin

if (len(streamingPlugin) == 0):
  print "Please specify path to streaming plugin using --plugin <filename>."
  sys.exit(1)
##########################################################################



## Start servermanager
servermanager.Connect()

## Load plugin
servermanager.LoadPlugin(streamingPlugin)

## Create streaming view
view = servermanager.rendering.StreamingRenderView()
'''
## Set streaming parameters
pxm = servermanager.ProxyManager()
helper = pxm.GetProxy("helpers", "StreamingHelperInstance")
helper.GetProperty("StreamedPasses").SetElement(0, numberOfPasses)
helper.GetProperty("EnableStreamMessages").SetElement(0, streamMessages)
helper.GetProperty("PieceCacheLimit").SetElement(0, cacheSize)
'''

## Disable progress printing if requested
if (printProgress == 0):
  servermanager.ToggleProgressPrinting()

## Set log buffer length
if (printLog):
  pm = servermanager.vtkProcessModule.GetProcessModule()
  pm.SetLogBufferLength(servermanager.ActiveConnection.ID, 1, 100000)

## Create reader and contour filter
mandel = servermanager.sources.ImageMandelbrotSource(WholeExtent=(0,100,0,100,0,100))
contour = servermanager.filters.Contour(Input=mandel)
contour.ContourValues = [50.5]

## Create representation
repr = servermanager.CreateRepresentation(contour, view);

# Set background color and camera position
view.Background = (.5,.5,.5)
view.CameraViewUp = [0, 1, 0]
view.CameraFocalPoint = [-0.8, 0, 0]
view.CameraPosition = [-0.8, 0, 6]

i = 0
stop = 0
while (not stop):
  view.StillRender()
  stop = view.GetDisplayDone()
  print "Render %d" % i
  i += 1

## One last render
view.StillRender()

## Print timing information
if (printLog):
  timerInfo = servermanager.vtkPVTimerInformation()
  pm = servermanager.vtkProcessModule.GetProcessModule()
  pm.GatherInformation(servermanager.ActiveConnection.ID,\
                      1, # DATA_SERVER \
                      timerInfo, \
                      pm.GetProcessModuleIDAsInt())

  for i in range(timerInfo.GetNumberOfLogs()):
      log = timerInfo.GetLog(i)
      totalTime = 0
      for line in log.split('\n'):
        print "Proc %d: " % i, line
        if (line.endswith("seconds") and not line.startswith(" ")):
          words = line.split()
          words.pop()
          totalTime += float(words.pop())
      print "Proc %d: Total time: %f" % (i, totalTime)

if (sleepTime > 0):
  import time
  time.sleep(sleepTime)



