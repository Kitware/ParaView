import shutil
import os.path
from paraview.simple import *
from paraview import smtesting
smtesting.ProcessCommandLineArguments()

canex2 = OpenDataFile(smtesting.DataDir + "/Testing/Data/can.ex2")
canex2.ElementVariables = ['EQPS']
canex2.PointVariables = ['DISPL', 'VEL', 'ACCL']
canex2.GlobalVariables = ['KE', 'XMOM', 'YMOM', 'ZMOM', 'NSTEPS', 'TMSTEP']
canex2.ElementBlocks = ['Unnamed block ID: 1 Type: HEX', 'Unnamed block ID: 2 Type: HEX']

# get animation scene
animationScene1 = GetAnimationScene()

# update animation scene based on data timesteps
animationScene1.UpdateAnimationUsingDataTimeSteps()

dname = os.path.join(smtesting.TempDir, "reload_reader")
shutil.rmtree(dname, ignore_errors=True)

# Save 10 timesteps.
fnames = []
for ts in range(10):
    fname = os.path.join(dname, "can_%d.vtm" % ts)
    fnames.append(fname)
    SaveData(fname, proxy=canex2)
    animationScene1.GoToNext()

canvtms = OpenDataFile(fnames)
assert len(canvtms.TimestepValues) == len(fnames)

for ts in range(10,20):
    fname = os.path.join(dname, "can_%d.vtm" % ts)
    fnames.append(fname)
    SaveData(fname, proxy=canex2)
    animationScene1.GoToNext()

ExtendFileSeries(canvtms)
assert len(canvtms.TimestepValues) == len(fnames)

# Remove temp dir is test passed.
shutil.rmtree(dname, ignore_errors=True)
