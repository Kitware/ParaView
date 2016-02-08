from paraview.simple import *
from paraview import coprocessing

outputfrequency = 1
reinjectionfrequency = 70

# ----------------------- CoProcessor definition -----------------------

def CreateCoProcessor():
  def _CreatePipeline(coprocessor, datadescription):
    class Pipeline:
      # state file generated using paraview version 4.4.0-117-ge0a3d77

      # ----------------------------------------------------------------
      # setup the data processing pipelines
      # ----------------------------------------------------------------

      #### disable automatic camera reset on 'Show'
      paraview.simple._DisableFirstRenderCameraReset()

      # create a new 'Line' for seed sources
      line1 = Line()
      line1.Point1 = [1., 1., 30.]
      line1.Point2 = [1., 64., 30.]

      # create a producer from a simulation input
      fullgrid_99pvtu = coprocessor.CreateProducer(datadescription, 'input')

      # create a new 'ParticlePath'
      # disable resetting the cache so that the particle path filter works in situ
      # and only updates from previously computed information.
      particlePath1 = InSituParticlePath(Input=fullgrid_99pvtu, SeedSource=line1, DisableResetCache=1)
      particlePath1.SelectInputVectors = ['POINTS', 'velocity']
      # don't save particle locations from previous time steps. they can take
      # up a surprising amount of memory for long running simulations.
      particlePath1.ClearCache = 1

      # if we're starting from a restarted simulation, the following are
      # used to specify the time step for the restarted simulation and
      # the input for the previously advected particles to continue
      # advecting them
      if datadescription.GetTimeStep() != 0:
        restartparticles = XMLPartitionedPolydataReader(FileName='particles_50.pvtp')
        particlePath1.RestartSource = restartparticles
        particlePath1.FirstTimeStep = datadescription.GetTimeStep()
        particlePath1.RestartedSimulation = 1

      # create a new 'Parallel PolyData Writer'
      parallelPolyDataWriter1 = servermanager.writers.XMLPPolyDataWriter(Input=particlePath1)

      # register the writer with coprocessor
      # and provide it with information such as the filename to use,
      # how frequently to write the data, etc.
      coprocessor.RegisterWriter(parallelPolyDataWriter1, filename='particles_%t.pvtp', freq=outputfrequency)

    return Pipeline()

  class CoProcessor(coprocessing.CoProcessor):
    def CreatePipeline(self, datadescription):
      self.Pipeline = _CreatePipeline(self, datadescription)

  coprocessor = CoProcessor()
  # these are the frequencies at which the coprocessor updates. for
  # particle paths this is done every time step
  freqs = {'input': [1]}
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
    #if not coprocessor.__PipelineCreated:
    coprocessor.UpdateProducers(datadescription)

    # tell the particle path filter how far to integrate in time (i.e. our current time)
    coprocessor.Pipeline.particlePath1.TerminationTime = datadescription.GetTime()

    # specify reinjection frequency manually so that reinjection
    # occurs based on the simulation time step to avoid restart issues since
    # the particle path filter only knows how many time steps
    # it has been updated. this is the same when the simulation has not been
    # restarted.
    timestep = datadescription.GetTimeStep()
    if timestep % reinjectionfrequency == 0:
      coprocessor.Pipeline.particlePath1.ForceReinjectionEveryNSteps = 1
    else:
      coprocessor.Pipeline.particlePath1.ForceReinjectionEveryNSteps = timestep+1

    coprocessor.Pipeline.particlePath1.UpdatePipeline()

    # Write output data, if appropriate.
    coprocessor.WriteData(datadescription);

    # Write image capture (Last arg: rescale lookup table), if appropriate.
    coprocessor.WriteImages(datadescription, rescale_lookuptable=False)

    # Live Visualization, if enabled.
    coprocessor.DoLiveVisualization(datadescription, "localhost", 22222)
