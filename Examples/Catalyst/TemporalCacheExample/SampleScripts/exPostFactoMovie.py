# A sample Catalyst Python pipeline with an ex post facto trigger.
# Here we wait until particles collide. Only then do we output anything.
# Furthermore, we output the timesteps that led up to the collision,
# so that we can reason about what the cause was.

from paraview.simple import *
from paraview import coprocessing

#--------------------------------------------------------------
# Code generated from cpstate.py to create the CoProcessor.


# ----------------------- CoProcessor definition -----------------------

def CreateCoProcessor():
  def _CreatePipeline(coprocessor, datadescription):
    class Pipeline:
      # KEY POINT:
      # We don't have to do anything special at each timestep.
      # But we do have to make the data available.
      # In this case the recent data.
      data = coprocessor.CreateTemporalProducer( datadescription, "volume" )

    return Pipeline()

  class CoProcessor(coprocessing.CoProcessor):
    def CreatePipeline(self, datadescription):
      self.Pipeline = _CreatePipeline(self, datadescription)
      # some bookkeeping structures to make a nice set of outputs
      self.StepsWritten = set()
      self.TimesToTimesteps = {}

    def ProcessTriggers(self, datadescription):
      # In this trigger we Look at the simulation's output, and see if we need to do anything
      # out of the ordinary. We have access to the N most recent outputs so we write them all out to disk.
      data = self.Pipeline.data
      timestep = datadescription.GetTimeStep()
      time = datadescription.GetTime()
      self.TimesToTimesteps[time] = timestep
      data.UpdatePipeline(time)
      orange = data.GetDataInformation().GetCellDataInformation().GetArrayInformation("occupancy").GetComponentRange(0)
      if orange[1] > 1:
        # a collision!
        print ("collision at tstep %d, time %f, range is [%f,%f]" % (timestep, time, orange[0], orange[1]))
        data.UpdatePipelineInformation()
        dtimes = data.TimestepValues
        for t in dtimes:
            if (t not in self.StepsWritten):
                print ("write ", t)
                self.StepsWritten.add(t)
                tstep = self.TimesToTimesteps[t]
                writer = servermanager.writers.XMLPImageDataWriter(Input=data)
                writer.FileName =  "tevent_%d.pvti" % tstep
                writer.UpdatePipeline(t)

  coprocessor = CoProcessor()
  freqs = {'volume': [10, 100]}
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
coprocessor.EnableLiveVisualization(True, 1)


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

    # KEY POINT:
    # We make up a trigger here that is evaluated on every timestep.
    coprocessor.ProcessTriggers(datadescription)

    # Write output data, if appropriate.
    coprocessor.WriteData(datadescription);

    # Write image capture (Last arg: rescale lookup table), if appropriate.
    coprocessor.WriteImages(datadescription, rescale_lookuptable=False)

    # Live Visualization, if enabled.
    coprocessor.DoLiveVisualization(datadescription, "localhost", 22222)
