from paraview.simple import *
from paraview.vtk.util.misc import vtkGetDataRoot

from os.path import realpath, join, dirname

scriptdir = dirname(realpath(__file__))
state1 = join(scriptdir, "iron-protein.pvsm")

datadir = join(vtkGetDataRoot(), "Testing/Data")

# load state with search under specified dir
LoadState(state1,
    LoadStateDataFileOptions='Search files under specified directory',
    DataDirectory=datadir)

# load state with specific filename
state2 = join(scriptdir, "blow-state.pvsm")
LoadState(state2,
    LoadStateDataFileOptions='Choose File Names',
    blowvtkFileNames=[join(datadir, 'blow.vtk')])
