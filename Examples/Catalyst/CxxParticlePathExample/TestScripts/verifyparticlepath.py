from __future__ import print_function
import sys

print('running the test script with args', sys.argv)

if len(sys.argv) != 2:
    print('need to pass in the test directory location')
    sys.exit(1)


# the velocity is [y*t, 0, 0] and the time step length is 0.018
# the beginning x-location for the seeds is 1
# the y location for the seeds are [1, 11.5, 22, 32.5, 43, 53.5, 64]
# the z location for the seeds is 30
# the analytical locations of the initially injected seeds are 1+y*time*time/2
# the seeds are reinjected at time step 70 (time 1.26) and their
# analytical locations are 1+y*(time*time-1.26*1.26)/2

# in parallel when seeds are migrated to different processes there
# is a slight loss of accuracy due to using a first order time
# integration method instead of vtkRungeKutta4.cxx.

from paraview.simple import *
r = XMLPartitionedPolydataReader(FileName=sys.argv[1]+'/particles_40.pvtp')
r.UpdatePipeline()
bounds = r.GetDataInformation().DataInformation.GetBounds()
if bounds[0] < 1.25 or bounds[0] > 1.27 or \
   bounds[1] < 17.5 or bounds[1] > 17.7 or \
   bounds[2] < .9 or bounds[2] > 1.1 or \
   bounds[3] < 63.9 or bounds[3] > 64.1 or \
   bounds[4] < 29.9 or bounds[4] > 30.1 or \
   bounds[5] < 29.9 or bounds[5] > 30.1:
    print('Time step 40: wrong particle bounds', bounds)
    sys.exit(1)


g = Glyph()
g.GlyphMode = 'All Points'
g.GlyphType = '2D Glyph'
g.GlyphType.GlyphType = 'Vertex'

t = Threshold()
t.Scalars = ['POINTS', 'ParticleAge']
t.ThresholdRange = [0.71, 0.73]
t.UpdatePipeline()

grid = servermanager.Fetch(t)
if grid.GetNumberOfPoints() != 7:
    print('Time step 40: wrong number of points', grid.GetNumberOfPoints())
    sys.exit(1)

r.FileName = sys.argv[1]+'/particles_80.pvtp'

# threshold to get the seeds that were originally injected only
t.ThresholdRange = [1.43, 1.45]
t.UpdatePipeline()
bounds = t.GetDataInformation().DataInformation.GetBounds()
if bounds[0] < 2. or bounds[0] > 2.1 or \
   bounds[1] < 67.3 or bounds[1] > 67.4 or \
   bounds[2] < .9 or bounds[2] > 1.1 or \
   bounds[3] < 63.9 or bounds[3] > 64.1 or \
   bounds[4] < 29.9 or bounds[4] > 30.1 or \
   bounds[5] < 29.9 or bounds[5] > 30.1:
    print('Time step 80: wrong particle bounds for initial injected particles', bounds)
    sys.exit(1)

grid = servermanager.Fetch(t)
if grid.GetNumberOfPoints() != 7:
    print('Time step 80: wrong number of points for initial injected particles', grid.GetNumberOfPoints())
    sys.exit(1)

# threshold to get the seeds that were injected at time step 70
t.Scalars = ['POINTS', 'InjectionStepId']
t.ThresholdRange = [69, 71]
t.UpdatePipeline()

bounds = t.GetDataInformation().DataInformation.GetBounds()
if bounds[0] < 1.23 or bounds[0] > 1.25 or \
   bounds[1] < 16.5 or bounds[1] > 16.7 or \
   bounds[2] < .9 or bounds[2] > 1.1 or \
   bounds[3] < 63.9 or bounds[3] > 64.1 or \
   bounds[4] < 29.9 or bounds[4] > 30.1 or \
   bounds[5] < 29.9 or bounds[5] > 30.1:
    print('Time step 80: wrong particle bounds for reinjected particles', bounds)
    sys.exit(1)

grid = servermanager.Fetch(t)
if grid.GetNumberOfPoints() != 7:
    print('Time step 80: wrong number of points for reinjected particles', grid.GetNumberOfPoints())
    sys.exit(1)

print('test passed')
