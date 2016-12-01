from paraview.simple import *

from paraview import smtesting
from paraview.vtk.vtkPVClientServerCoreRendering import vtkPVCacheKeeper
from paraview.vtk.vtkPVServerManagerDefault import vtkPVGeneralSettings

smtesting.ProcessCommandLineArguments()

filename = smtesting.DataDir + '/can.ex2'
can_ex2 = OpenDataFile(filename)

AnimationScene1 = GetAnimationScene()
AnimationScene1.EndTime = 0.004299988504499197
AnimationScene1.PlayMode = 'Snap To TimeSteps'

can_ex2.PointVariables = ['ACCL', 'DISPL', 'VEL']
can_ex2.ElementVariables = ['EQPS']
can_ex2.GlobalVariables = ['KE', 'NSTEPS', 'TMSTEP', 'XMOM', 'YMOM', 'ZMOM']
can_ex2.ApplyDisplacements = 0
can_ex2.ElementBlocks = ['Unnamed block ID: 1 Type: HEX Size: 4800', 'Unnamed block ID: 2 Type: HEX Size: 2352']

DataRepresentation1 = Show()
RenderView1 = GetRenderView()

RenderView1.CameraPosition = [0.21706008911132812, 4.0, 46.629626614745732]
RenderView1.CameraFocalPoint = [0.21706008911132812, 4.0, -5.1109471321105957]
RenderView1.CameraParallelScale = 13.391445890217907
RenderView1.CenterOfRotation = [0.21706008911132812, 4.0, -5.1109471321105957]

RenderView1.CameraViewUp = [0.0, 0.0, 1.0]
RenderView1.CameraPosition = [0.21706008911132812, 55.740573746856327, -5.1109471321105957]
RenderView1.CameraFocalPoint = [0.21706008911132812, 4.0, -5.1109471321105957]

Render()

#---------------------------------------------------------
# Initial state, nothing cached, or expected to be cached.
vtkPVCacheKeeper.ClearCacheStateFlags()
AnimationScene1.GoToNext()
assert vtkPVCacheKeeper.GetCacheSkips() > 0 and vtkPVCacheKeeper.GetCacheMisses() == 0 and vtkPVCacheKeeper.GetCacheHits() == 0

#---------------------------------------------------------
# nothing cached, or expected to be cached since we didn't enable caching.
vtkPVCacheKeeper.ClearCacheStateFlags()
AnimationScene1.GoToNext()
AnimationScene1.GoToNext()
AnimationScene1.GoToPrevious()
assert vtkPVCacheKeeper.GetCacheSkips() > 0 and vtkPVCacheKeeper.GetCacheMisses() == 0 and vtkPVCacheKeeper.GetCacheHits() == 0

#---------------------------------------------------------
# Enable caching
vtkPVGeneralSettings.GetInstance().SetCacheGeometryForAnimation(True)

#---------------------------------------------------------
# Nothing in cache, but we expect the data to be cached for next time.
vtkPVCacheKeeper.ClearCacheStateFlags()
AnimationScene1.GoToFirst()
assert vtkPVCacheKeeper.GetCacheSkips() == 0 and vtkPVCacheKeeper.GetCacheMisses() > 0 and vtkPVCacheKeeper.GetCacheHits() == 0

vtkPVCacheKeeper.ClearCacheStateFlags()
AnimationScene1.GoToNext()
assert vtkPVCacheKeeper.GetCacheSkips() == 0 and vtkPVCacheKeeper.GetCacheMisses() > 0 and vtkPVCacheKeeper.GetCacheHits() == 0

#---------------------------------------------------------
# now we expect the cache to hit and have no misses.
vtkPVCacheKeeper.ClearCacheStateFlags()
AnimationScene1.GoToPrevious()
AnimationScene1.GoToFirst()
assert vtkPVCacheKeeper.GetCacheSkips() == 0 and vtkPVCacheKeeper.GetCacheMisses() == 0 and vtkPVCacheKeeper.GetCacheHits() > 0

#---------------------------------------------------------
# Let's fill up the cache fully. We'll have some hits and misses, but no skips.
vtkPVCacheKeeper.ClearCacheStateFlags()
AnimationScene1.GoToFirst()
AnimationScene1.Play()
assert vtkPVCacheKeeper.GetCacheSkips() == 0 and vtkPVCacheKeeper.GetCacheMisses() > 0 and vtkPVCacheKeeper.GetCacheHits() > 0


#---------------------------------------------------------
# Play again, this time cache should be used.
vtkPVCacheKeeper.ClearCacheStateFlags()
AnimationScene1.Play()
assert vtkPVCacheKeeper.GetCacheSkips() == 0 and vtkPVCacheKeeper.GetCacheMisses() == 0 and vtkPVCacheKeeper.GetCacheHits() > 0

#---------------------------------------------------------
# Modify properties and play. this time it should be a cache miss since
# the cache should have been cleared.
can_ex2.PointVariables = ['ACCL', 'DISPL']
vtkPVCacheKeeper.ClearCacheStateFlags()
AnimationScene1.Play()
assert vtkPVCacheKeeper.GetCacheSkips() == 0 and vtkPVCacheKeeper.GetCacheMisses() > 0 and vtkPVCacheKeeper.GetCacheHits() == 0

print("All's well that ends well! Looks like the cache is working as expected.")
