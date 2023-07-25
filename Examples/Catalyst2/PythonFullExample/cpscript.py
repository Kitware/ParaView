from paraview.simple import *
from paraview import coprocessing

#--------------------------------------------------------------
# Code generated from cpstate.py to create the CoProcessor.


# ----------------------- CoProcessor definition -----------------------

def CreateCoProcessor():
  def _CreatePipeline(coprocessor, datadescription):
    class Pipeline:
      filename_6_pvti = coprocessor.CreateProducer( datadescription, "input" )

      # create a new 'Parallel ImageData Writer'
      imageDataWriter1 = servermanager.writers.XMLPImageDataWriter(Input=filename_6_pvti)

      # register the writer with coprocessor
      # and provide it with information such as the filename to use,
      # how frequently to write the data, etc.
      coprocessor.RegisterWriter(imageDataWriter1, filename="fullgrid_%t.pvti", freq=100)

      Slice1 = Slice( Input=filename_6_pvti, guiName="Slice1", Crinkleslice=0, SliceOffsetValues=[0.0], Triangulatetheslice=1, SliceType="Plane" )
      Slice1.SliceType.Offset = 0.0
      Slice1.SliceType.Origin = [9.0, 11.0, 9.0]
      Slice1.SliceType.Normal = [1.0, 0.0, 0.0]

      # create a new 'Parallel PolyData Writer'
      parallelPolyDataWriter1 = servermanager.writers.XMLPPolyDataWriter(Input=Slice1)

      # register the writer with coprocessor
      # and provide it with information such as the filename to use,
      # how frequently to write the data, etc.
      coprocessor.RegisterWriter(parallelPolyDataWriter1, filename='slice_%t.pvtp', freq=10)

    return Pipeline()

  class CoProcessor(coprocessing.CoProcessor):
    def CreatePipeline(self, datadescription):
      self.Pipeline = _CreatePipeline(self, datadescription)

  coprocessor = CoProcessor()
  freqs = {'input': [10, 100]}
  coprocessor.SetUpdateFrequencies(freqs)
  return coprocessor

#--------------------------------------------------------------
# Global variables that will hold the pipeline for each timestep
# Creating the CoProcessor object, doesn't actually create the ParaView pipeline.
# It will be automatically setup when coprocessor.UpdateProducers() is called the
# first time.
coprocessor = CreateCoProcessor()

#--------------------------------------------------------------
# Enable Live-Visualizaton with ParaView
coprocessor.EnableLiveVisualization(False, 1)


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
    coprocessor.WriteData(datadescription);

    # Write image capture (Last arg: rescale lookup table), if appropriate.
    coprocessor.WriteImages(datadescription, rescale_lookuptable=False)

    # Live Visualization, if enabled.
    coprocessor.DoLiveVisualization(datadescription, "localhost", 22222)
