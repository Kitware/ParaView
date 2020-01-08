
from paraview.simple import *
from paraview import smtesting
from os.path import join
import os, shutil

def Barrier():
    # ensure all ranks wait till root has created the directory to write into.
    pm = servermanager.vtkProcessModule.GetProcessModule()
    if pm.GetSymmetricMPIMode():
        pm.GetGlobalController().Barrier()

def InitializeDir(rootdir, create=True):
    pm = servermanager.vtkProcessModule.GetProcessModule()
    if pm.GetPartitionId() == 0:
        shutil.rmtree(rootdir, ignore_errors=True)
        if create:
            os.makedirs(rootdir)
    Barrier()


smtesting.ProcessCommandLineArguments()

pm = servermanager.vtkProcessModule.GetProcessModule()
# separate dirs to avoid failures in parallel test runs
if pm.GetSymmetricMPIMode():
    rootdir = join(smtesting.TempDir, "parallelserialwritermultiplerankio-sym")
else:
    rootdir = join(smtesting.TempDir, "parallelserialwritermultiplerankio")

fname = join(rootdir, "split-sphere.stl")
InitializeDir(rootdir)

s = Sphere()
s.PhiResolution = 80
s.ThetaResolution = 80

SaveData(join(rootdir, "sphere-cont.stl"), s, NumberOfIORanks=2, RankAssignmentMode="Contiguous")
SaveData(join(rootdir, "sphere-rr.stl"), s, NumberOfIORanks=2, RankAssignmentMode="RoundRobin")


Barrier()
# now read the files and render the result

c0 = OpenDataFile(join(rootdir, "sphere-cont-0.stl"))
c1 = OpenDataFile(join(rootdir, "sphere-cont-1.stl"))
grc = GroupDatasets(Input=[c0, c1])
dc = Show(grc)

r0 = OpenDataFile(join(rootdir, "sphere-rr-0.stl"))
r1 = OpenDataFile(join(rootdir, "sphere-rr-1.stl"))
grr = GroupDatasets(Input=[r0, r1])
dr = Show(grr)
dr.Position = [1, 1, 0]

ResetCamera()
ColorBy(dc, ('FIELD', 'vtkBlockColors'))
ColorBy(dr, ('FIELD', 'vtkBlockColors'))

view = Render()
Barrier()

smtesting.DoRegressionTesting(view.SMProxy)
if not smtesting.DoRegressionTesting(view.SMProxy):
    raise smtesting.TestError('Test failed.')

# remove dirs on success
InitializeDir(rootdir, create=False)
