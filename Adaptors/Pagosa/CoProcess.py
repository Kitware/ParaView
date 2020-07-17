from paraview.simple import *
from paraview import coprocessing
import os

if   os.getenv("DUMP_PATH") == None:
  dumpPath = os.getenv("PWD")
elif os.getenv("DUMP_PATH") == "":
  dumpPath = os.getenv("PWD")
else:
  dumpPath = os.getenv("DUMP_PATH")

#--------------------------------------------------------------
# Code generated from cpstate.py to create the CoProcessor.

# ----------------------- CoProcessor definition -----------------------

def CreateCoProcessor():
  def _CreatePipeline(coprocessor, datadescription):
    class Pipeline:
      adaptorinput = coprocessor.CreateProducer( datadescription, "GridCellData" )
      grid = adaptorinput.GetClientSideObject().GetOutputDataObject(0)
      fileName = dumpPath + "/insitu/GridCellData_%t.vth"
      writer = coprocessor.CreateWriter( XMLHierarchicalBoxDataWriter, fileName, 1 )
#     writer = coprocessor.CreateWriter(vtkXMLPUniformGridAMRWriter, fileName, 1 )

      adaptorinput2 = coprocessor.CreateProducer( datadescription, "MarkerPointData" )
      grid2 = adaptorinput2.GetClientSideObject().GetOutputDataObject(0)
      fileName2 = dumpPath + "/insitu/MarkerPointData_%t.vtm"
      writer2 = coprocessor.CreateWriter( XMLMultiBlockDataWriter, fileName2, 1 )

    return Pipeline()

  class CoProcessor(coprocessing.CoProcessor):
    def CreatePipeline(self, datadescription):
      self.Pipeline = _CreatePipeline(self, datadescription)

  coprocessor = CoProcessor()
# freqs = {"input": [1], "input2": [1]}
  coprocessor.SetUpdateFrequencies({"GridCellData":[1], "MarkerPointData":[1]})
  return coprocessor

#--------------------------------------------------------------
# Global variables that will hold the pipeline for each timestep
# Creating the CoProcessor object, doesn't actually create the ParaView pipeline.
# It will be automatically setup when coprocessor.UpdateProducers() is called the
# first time.
coprocessor = CreateCoProcessor()

#--------------------------------------------------------------
# Enable Live-Visualizaton with ParaView
coprocessor.EnableLiveVisualization(False)

# ---------------------- Data Selection method ----------------------

def RequestDataDescription(datadescription):
    "Callback to populate the request for current timestep"
    global coprocessor
    if datadescription.GetForceOutput() == True:
      # We are just going to request all fields and meshes from the simulation
      # code/adaptor.
      for i in range(datadescription.GetNumberOfInputDescriptions()):
          datadescription.GetInputDescription(i).AllFieldsOn()
          datadescription.GetInputDescription(i).GenerateMeshOn()
      return

    # setup requests for all inputs based on the requirements of the
    # pipeline.
    coprocessor.LoadRequestedData(datadescription)

# ------------------------ Processing method ------------------------

def DoCoProcessing(datadescription):
    "Callback to do co-processing for current timestep"
    global coprocessor

    # Update the coprocessor by providing it the newly generated simulation data.
    # If the pipeline hasn't been setup yet, this will setup the pipeline.
    coprocessor.UpdateProducers(datadescription)

    # Write output data, if appropriate.
#   print 'WRITE VTM FILES ',datadescription.GetTimeStep()
    coprocessor.WriteData(datadescription);

    # Write image capture (Last arg: rescale lookup table), if appropriate.
    coprocessor.WriteImages(datadescription, rescale_lookuptable=False)

    # Live Visualization, if enabled.
    coprocessor.DoLiveVisualization(datadescription, "localhost", 22222)
