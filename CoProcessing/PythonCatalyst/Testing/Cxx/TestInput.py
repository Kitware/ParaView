from paraview import coprocessing, servermanager


# ----------------------- CoProcessor definition -----------------------
def CreateCoProcessor():
  def _CreatePipeline(coprocessor, datadescription):
    class Pipeline:
      Wavelet1 = coprocessor.CreateProducer( datadescription, "input" )
    return Pipeline()

  class CoProcessor(coprocessing.CoProcessor):
    def CreatePipeline(self, datadescription):
      self.Pipeline = _CreatePipeline(self, datadescription)

  coprocessor = CoProcessor()
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
    timestep = datadescription.GetTimeStep()
    print("Timestep: %d Time: %f" % ( timestep, datadescription.GetTime()))

    # Update the coprocessor by providing it the newly generated simulation data.
    # If the pipeline hasn't been setup yet, this will setup the pipeline.
    coprocessor.UpdateProducers(datadescription)

    pipeline = coprocessor.Pipeline

    grid = servermanager.Fetch(pipeline.Wavelet1)
    array = grid.GetPointData().GetArray("RTData")
    array_range = array.GetRange()
    if timestep == 0:
      if array_range[0] < 37 or array_range[0] > 38 or array_range[1] < 276 or array_range[1] > 277:
        print('ERROR: bad range of %f for step 0' % array_range)
        sys.exit(1)
    if timestep == 1:
      if array_range[0] < 74 or array_range[0] > 76 or array_range[1] < 443 or array_range[1] > 445:
        print('ERROR: bad range of %f for step 1' % array_range)
        sys.exit(1)
    if timestep == 2:
      if array_range[0] < 77 or array_range[0] > 79 or array_range[1] < 357 or array_range[1] > 458:
        print('ERROR: bad range of %f for step 2' % array_range)
        sys.exit(1)
    if timestep == 3:
      if array_range[0] < -43 or array_range[0] > 44 or array_range[1] < 304 or array_range[1] > 305:
        print('ERROR: bad range of %f for step 3' % array_range)
        sys.exit(1)
