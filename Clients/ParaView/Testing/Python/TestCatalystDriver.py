import paraview
paraview.options.batch = True
paraview.options.symmetric = True

import paraview.simple as pvsimple
from paraview.modules import vtkPVCatalyst, vtkPVPythonCatalyst
pm = pvsimple.servermanager.vtkProcessModule.GetProcessModule()
rank = pm.GetPartitionId()
nranks = pm.GetNumberOfLocalPartitions()

import numpy as np
NB_TIMESTEPS = 10
CHANNEL_NAME = 'input'

coProcessor = vtkPVCatalyst.vtkCPProcessor()
pipeline = vtkPVPythonCatalyst.vtkCPPythonStringPipeline()
# simple script that only perform live visu
pipeline.Initialize("""
from paraview.simple import *
from paraview import coprocessing

class CustomCoProcessor(coprocessing.CoProcessor):
    def CreatePipeline(self, datadescription):
        adaptorinput = coprocessor.CreateProducer( datadescription, "input" )

coprocessor = CustomCoProcessor()
coprocessor.EnableLiveVisualization(True, 1)
coprocessor.SetUpdateFrequencies({"input": [1]})

def RequestDataDescription(datadescription):
    global coprocessor
    coprocessor.LoadRequestedData(datadescription)

def DoCoProcessing(datadescription):
    global coprocessor
    coprocessor.UpdateProducers(datadescription)
    coprocessor.DoLiveVisualization(datadescription, "localhost", 22222)
""")
coProcessor.AddPipeline(pipeline)

print ("start simulation ...")
for step in np.arange(NB_TIMESTEPS):
    time = step / NB_TIMESTEPS
    datadescription = vtkPVCatalyst.vtkCPDataDescription()
    datadescription.SetTimeData(time, step)
    datadescription.AddInput(CHANNEL_NAME)
    if step == NB_TIMESTEPS-1:
         # last time step so we force the output
        datadescription.ForceOutputOn()

    if coProcessor.RequestDataDescription(datadescription) == 1:
        wavelet = pvsimple.Wavelet()
        wavelet.Maximum = 5 + 100 * time
        wavelet.UpdatePipeline()
        extent = wavelet.WholeExtent
        grid = pvsimple.servermanager.Fetch(wavelet)

        inputdescription = datadescription.GetInputDescriptionByName(CHANNEL_NAME)
        inputdescription.SetGrid(grid)
        inputdescription.SetWholeExtent(extent)

    coProcessor.CoProcess(datadescription)

print("simulation ended!")
coProcessor.Finalize()
