##
## Test validates the cinema's Azimuth-Elevation-Roll
## camera model export functions.
##

#### import the simple module from the paraview
from paraview.simple import *
#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

from vtk import *
testDir = vtk.util.misc.vtkGetTempDir()
import os

cinemaDBFileName = os.path.join(testDir, "cinema_aer.cdb")
# create a new 'Time Source'
timeSource1 = TimeSource()

# get animation scene
animationScene1 = GetAnimationScene()

# update animation scene based on data timesteps
animationScene1.UpdateAnimationUsingDataTimeSteps()

# get active view
renderView1 = GetActiveViewOrCreate('RenderView')
# uncomment following to set a specific view size
renderView1.ViewSize = [300, 300]

# show data in view
timeSource1Display = Show(timeSource1, renderView1)
# trace defaults for the display properties.
timeSource1Display.ColorArrayName = [None, '']

# reset view to fit data
renderView1.ResetCamera()

# Properties modified on timeSource1
timeSource1.Growing = 1

# current camera placement for renderView1
renderView1.CameraPosition = [0.48740174980560036, 2.1999724794111692, 11.799031020840467]
renderView1.CameraFocalPoint = [0.48740174980560036, 2.1999724794111692, 8.452965805889235]
renderView1.CameraParallelScale = 0.8660254037844386

# export view
ExportView(cinemaDBFileName, view=renderView1, ViewSelection='\'RenderView1\' : [\'image_%t.png\', 1, 0, 1, 569, 427, {"composite":True, "camera":"azimuth-elevation-roll", "phi":[3],"theta":[3], "roll":[3], "initial":{ "eye": [0.487402,2.19997,11.799], "at": [0.5,0.5,0.5], "up": [0,1,0] }, "tracking":{ "object":"TimeSource1" } }]',
    TrackSelection='',
    ArraySelection="'timeSource1' : ['Point Value', 'Point X']")

#### saving camera placements for all active views

#### uncomment the following to render all views
# RenderAllViews()
# alternatively, if you want to write images, you can use SaveScreenshot(...).

import exceptions
resultfname = os.path.join(cinemaDBFileName, "image/info.json")
if not os.path.isfile(resultfname):
    raise exceptions.RuntimeError, "cinema index file not written "

resultfname = os.path.join(cinemaDBFileName, "image/pose=0/time=0/vis=0/colorTimeSource1=0.Z")
if not os.path.isfile(resultfname):
    raise exceptions.RuntimeError, "cinema raster file not written"

import shutil
shutil.rmtree(cinemaDBFileName)
