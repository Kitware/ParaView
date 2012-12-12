
try: paraview.simple
except: from paraview.simple import *
import sys

# Make sure the helper methods are loaded
try:
  __cp_helper_script_loaded__
except:
  import vtkCoProcessorPython
  exec vtkCoProcessorPython.vtkCPHelperScripts.GetPythonHelperScript()

# Global variables that will hold the pipeline for each timestep
pipeline = None

# Live visualization inside ParaView
live_visu_active = False
pv_host = "localhost"
pv_port = 22222

write_frequencies    = {'input': [1]}
simulation_input_map = {'Wavelet1': 'input'}

# ----------------------- Pipeline definition -----------------------

def CreatePipeline(datadescription):
  class Pipeline:
    global cp_views, cp_writers
    Wavelet1 = CreateProducer( datadescription, "input" )

  return Pipeline()


# ---------------------- Data Selection method ----------------------

def RequestDataDescription(datadescription):
    "Callback to populate the request for current timestep"
    if datadescription.GetForceOutput() == True:
        for i in range(datadescription.GetNumberOfInputDescriptions()):
            datadescription.GetInputDescription(i).AllFieldsOn()
            datadescription.GetInputDescription(i).GenerateMeshOn()
        return

    for input_name in simulation_input_map.values():
       LoadRequestedData(datadescription, input_name)

# ------------------------ Processing method ------------------------

def DoCoProcessing(datadescription):
    "Callback to do co-processing for current timestep"
    global pipeline, cp_writers, cp_views
    timestep = datadescription.GetTimeStep()

    # Load the Pipeline if not created yet
    if not pipeline:
       pipeline = CreatePipeline(datadescription)
    else:
      # update to the new input and time
      UpdateProducers(datadescription)

    grid = servermanager.Fetch(pipeline.Wavelet1)
    array = grid.GetPointData().GetArray("RTData")
    array_range = array.GetRange()
    if timestep == 0:
      if array_range[0] < 37 or array_range[0] > 38 or array_range[1] < 276 or array_range[1] > 277:
        print 'ERROR: bad range of ', array_range, ' for step 0'
        sys.exit(1)
    if timestep == 1:
      if array_range[0] < 74 or array_range[0] > 76 or array_range[1] < 443 or array_range[1] > 445:
        print 'ERROR: bad range of ', array_range, ' for step 1'
        sys.exit(1)
    if timestep == 2:
      if array_range[0] < 77 or array_range[0] > 79 or array_range[1] < 357 or array_range[1] > 458:
        print 'ERROR: bad range of ', array_range, ' for step 2'
        sys.exit(1)
    if timestep == 3:
      if array_range[0] < -43 or array_range[0] > 44 or array_range[1] < 304 or array_range[1] > 305:
        print 'ERROR: bad range of ', array_range, ' for step 3'
        sys.exit(1)
