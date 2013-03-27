try: paraview.simple
except: from paraview.simple import *

# Make sure the helper methods are loaded
try:
  __cp_helper_script_loaded__
except:
  import vtkPVPythonCatalystPython
  exec vtkPVPythonCatalystPython.vtkCPPythonHelper.GetPythonHelperScript()

# Global variables that will hold the pipeline for each timestep
pipeline = None

# Live visualization inside ParaView
live_visu_active = False
pv_host = "localhost"
pv_port = 22222

write_frequencies    = {'input': [5, 20]}
simulation_input_map = {'Wavelet1': 'input'}

# ----------------------- Pipeline definition -----------------------

def CreatePipeline(datadescription):
  class Pipeline:
    global cp_views, cp_writers
    Wavelet1 = CreateProducer( datadescription, "input" )

    ParallelImageDataWriter1 = CreateCPWriter( XMLPImageDataWriter, "fullgrid_%t.pvti", 20, cp_writers )

    SetActiveSource(Wavelet1)
    Slice1 = Slice( guiName="Slice1", Crinkleslice=0, SliceOffsetValues=[0.0], Triangulatetheslice=1, SliceType="Plane" )
    Slice1.SliceType.Offset = 0.0
    Slice1.SliceType.Origin = [49.5, 49.5, 49.5]
    Slice1.SliceType.Normal = [1.0, 0.0, 0.0]

    ParallelPolyDataWriter1 = CreateCPWriter( XMLPPolyDataWriter, "slice_%t.pvtp", 5, cp_writers )

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

    # Write output data
    WriteAllData(datadescription, cp_writers, timestep);

    # Write image capture (Last arg: rescale lookup table)
    WriteAllImages(datadescription, cp_views, timestep, False)

    # Live Visualization
    if live_visu_active:
       DoLiveInsitu(timestep, pv_host, pv_port)

