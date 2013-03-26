# sample Python script for CxxFullExample. Run with:
# <FEDriver path>/FEDriver <path to this script>/feslicescript.py

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

write_frequencies    = {'input': [10, 100]}
simulation_input_map = {'filename_*': 'input'}

# ----------------------- Pipeline definition -----------------------

def CreatePipeline(datadescription):
  class Pipeline:
    global cp_views, cp_writers
    filename_ = CreateProducer( datadescription, "input" )

    Slice2 = Slice( guiName="Slice2", Crinkleslice=0, SliceOffsetValues=[-3.0, -1.0, 1.0, 3.0], Triangulatetheslice=1, SliceType="Plane" )
    Slice2.SliceType.Offset = 0.0
    Slice2.SliceType.Origin = [2.0, 2.75, 3.9000000000000004]
    Slice2.SliceType.Normal = [0.0, 0.0, 1.0]

    SetActiveSource(filename_)
    ParallelImageDataWriter1 = CreateCPWriter( XMLUniformGridAMRWriter, "fullgrid_%t.vth", 100, cp_writers )

    SetActiveSource(Slice2)
    ParallelPolyDataWriter1 = CreateCPWriter( XMLMultiBlockDataWriter, "slice_%t.vtm", 10, cp_writers )

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

