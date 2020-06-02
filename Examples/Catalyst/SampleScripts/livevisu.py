from paraview.simple import *
from paraview import coprocessing

class CustomCoProcessor(coprocessing.CoProcessor):
    def CreatePipeline(self, datadescription):
        adaptorinput = coprocessor.CreateProducer( datadescription, "input" )

coprocessor = CustomCoProcessor()
coprocessor.EnableLiveVisualization(True, 1)
coprocessor.SetUpdateFrequencies({'input': [1]})

def RequestDataDescription(datadescription):
    global coprocessor
    coprocessor.LoadRequestedData(datadescription)

def DoCoProcessing(datadescription):
    global coprocessor
    coprocessor.UpdateProducers(datadescription)
    coprocessor.DoLiveVisualization(datadescription, "localhost", 22222)
