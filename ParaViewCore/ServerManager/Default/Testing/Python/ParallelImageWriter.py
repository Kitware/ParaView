import sys

import SMPythonTesting
import os
import os.path
import sys
from paraview.simple import *

paraview.simple._DisableFirstRenderCameraReset()

SMPythonTesting.ProcessCommandLineArguments()

fname = os.path.join(SMPythonTesting.TempDir, "parallelimagewritertest.pvti")

import paraview.servermanager
pm = paraview.servermanager.vtkProcessModule.GetProcessModule()

# if the file exists, delete it on process 0 just to be safe
processId = pm.GetGlobalController().GetLocalProcessId()
if processId == 0:
    if os.path.isfile(fname) == True:
        os.remove(fname)

Wavelet1 = Wavelet()
w = XMLPImageDataWriter()
w.FileName = fname
w.UpdatePipeline()

r = XMLPartitionedImageDataReader()
r.FileName = fname
r.UpdatePipeline()

# numcells is the local value, we need to get the global value
numcells =  r.GetDataInformation().DataInformation.GetNumberOfCells()

# sum up the cell count on process 0 and process 0 checks it
import paraview.vtk as vtk
da = vtk.vtkIntArray()
da.SetNumberOfTuples(1)
da.SetValue(0, numcells)
da2 = vtk.vtkIntArray()
da2.SetNumberOfTuples(1)

pm.GetGlobalController().Reduce(da, da2, 2, 0)

if processId == 0 and da2.GetValue(0) != 8000:
    print "ERROR: ", fname, " has ", da2.GetValue(0), " but should have 8000."
    sys.exit(1)

print "Test passed."
