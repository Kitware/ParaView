"""
A Catalyst pipeline Python script that writes out the
full grid and all attributes independent of the grid
type created by the adaptor. It works in parallel and
writes to a file called filename_%t.<ext>" where %t is
the time step and <ext> is the appropriate file name
extension for the data set. This script is useful if
there is no existing data set to create a Catalyst
pipeline from.
"""
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

write_frequencies    = {'input': [1]}
simulation_input_map = {'adaptorinput': 'input'}

# ----------------------- Pipeline definition -----------------------

def CreatePipeline(datadescription):
  class Pipeline:
    global cp_views, cp_writers
    adaptorinput = CreateProducer( datadescription, "input" )
    adaptorinput.UpdatePipeline()
    gridtype = adaptorinput.GetDataInformation().DataInformation.GetDataClassName()
    if  gridtype == 'vtkImageData' or gridtype == 'vtkUniformGrid':
      writer = CreateCPWriter( XMLPImageDataWriter, "filename_%t.pvti", 1, cp_writers )
    elif  gridtype == 'vtkRectilinearGrid':
      writer = CreateCPWriter( XMLPRectilinearGridWriter, "filename_%t.pvtr", 1, cp_writers )
    elif  gridtype == 'vtkStructuredGrid':
      writer = CreateCPWriter( XMLPStructuredGridWriter, "filename_%t.pvts", 1, cp_writers )
    elif  gridtype == 'vtkPolyData':
      writer = CreateCPWriter( XMLPPolyDataWriter, "filename_%t.pvtp", 1, cp_writers )
    elif  gridtype == 'vtkUnstructuredGrid':
      writer = CreateCPWriter( XMLPUnstructuredGridWriter, "filename_%t.pvtu", 1, cp_writers )
    elif  gridtype == 'vtkHierarchicalBoxDataSet':
      writer = CreateCPWriter( XMLHierarchicalBoxDataWriter, "filename_%t.vthb", 1, cp_writers )
    elif  gridtype == 'vtkMultiBlockDataSet':
      writer = CreateCPWriter( XMLMultiBlockDataWriter, "filename_%t.vtm", 1, cp_writers )
    else:
      print "Don't know how to create a writer for a ", gridtype
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
