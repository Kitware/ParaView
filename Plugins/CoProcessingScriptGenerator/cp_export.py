# -----------------------------------------------------------------------------
# This file is responsible to export a Python based script for the CoProcessing
# library that could be used later on. It is basically a template that get
# adapted to reproduce the visualization pipeline made inside ParaView on
# a smaller dataset before hand.
# -----------------------------------------------------------------------------

# boolean telling if we want to export rendering.
export_rendering = %1

# string->string map with key being the proxyname while value being the
# simulation input name.
simulation_input_map = { %2 };

# Other argument that get replaced from the C++
screenshot_info = { %3 }

# Do we want to rescale the data range at each timestep
rescale_data_range = %4

# File path that we want to generate
fileName = "%5"

# -----------------------------------------------------------------------------
# The output_contents is the templet script that accept 2 arguments:
#  1) The Pipeline class definition
#  2) The boolean to know if we need to rescale the data range
# -----------------------------------------------------------------------------

output_contents = """
try: paraview.simple
except: from paraview.simple import *

# Make sure the helper methods are loaded
try:
  __cp_helper_script_loaded__
except:
  import vtkCoProcessorPython
  exec vtkCoProcessorPython.vtkCPHelperScripts.GetPythonHelperScript()

# Global variables that will hold the pipeline for each timestep
pipeline   = None

%s

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

    # Write output data
    WriteAllData(datadescription, cp_writers, timestep);

    # rescale data range ?
    if %s:
        RescaleDataRange(datadescription, cp_views, timestep)

    # Write image capture
    WriteAllImages(datadescription, cp_views, timestep)

    # explicitly delete the proxies -- we do it this way to avoid problems with prototypes
    # (Not sure we still need it) CleanupProxies()

"""

# -----------------------------------------------------------------------------
# Script file generation
# -----------------------------------------------------------------------------

outFile = open(fileName, 'w')
outFile.write(output_contents % \
  ( DumpPipeline(export_rendering, simulation_input_map, screenshot_info), \
    rescale_data_range))
outFile.close()
