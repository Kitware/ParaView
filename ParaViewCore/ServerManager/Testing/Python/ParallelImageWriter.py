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

numcells =  r.GetDataInformation().DataInformation.GetNumberOfCells()

#only process 0 has the global cell count
if processId == 0 and numcells != 8000:
    print "ERROR: ", fname, " has ", \
        r.GetDataInformation().DataInformation.GetNumberOfCells(), \
        " but should have 8000."
    sys.exit(1)

print "Test passed."
