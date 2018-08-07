r"""This module is used to export complete CoProcessing Python scripts that
can be used in a vtkCPPythonScriptPipeline.

This module uses paraview.cpstate Module to dump the ParaView session state as a
an Python class description that can be then be used in the CoProcessor.

The exported script can be used in a vtkCPPythonScriptPipeline instance for
CoProcessing."""

# -----------------------------------------------------------------------------
# The __output_contents is the templet script that accept 3 arguments:
#  1) The CoProcessor class definition
#  2) The boolean to know if we want to enable live-visualization
#  3) The boolean to know if we need to rescale the data range
# -----------------------------------------------------------------------------
__output_contents = """
from paraview.simple import *
from paraview import coprocessing


#--------------------------------------------------------------
# Code generated from cpstate.py to create the CoProcessor.
# %s

#--------------------------------------------------------------
# Global timestep output options
timeStepToStartOutputAt=%s
forceOutputAtFirstCall=%s

#--------------------------------------------------------------
# Global screenshot output options
imageFileNamePadding=%s
rescale_lookuptable=%s

#--------------------------------------------------------------
# whether or not to request specific arrays from the adaptor
requestSpecificArrays=%s

%s

#--------------------------------------------------------------
# Global variable that will hold the pipeline for each timestep
# Creating the CoProcessor object, doesn't actually create the ParaView pipeline.
# It will be automatically setup when coprocessor.UpdateProducers() is called the
# first time.
coprocessor = CreateCoProcessor()

#--------------------------------------------------------------
# Enable Live-Visualizaton with ParaView and the update frequency
coprocessor.EnableLiveVisualization(%s, %s)

# ---------------------- Data Selection method ----------------------

def RequestDataDescription(datadescription):
    "Callback to populate the request for current timestep"
    global coprocessor

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
    coprocessor.WriteImages(datadescription, rescale_lookuptable=rescale_lookuptable,
        image_quality=0, padding_amount=imageFileNamePadding)

    # Live Visualization, if enabled.
    coprocessor.DoLiveVisualization(datadescription, "localhost", 22222)
"""

from paraview import cpstate

def DumpCoProcessingScript(export_rendering, simulation_input_map, screenshot_info,
    padding_amount, rescale_data_range, enable_live_viz, live_viz_frequency,
    cinema_tracks, cinema_arrays, filename=None):
    """Returns a string with the generated CoProcessing script based on the
    options specified.

    First three arguments are same as those expected by
    cpstate.DumpPipeline() function.

    :param padding_amount: integer (0-10) to set image output filename padding

    :param rescale_data_range: boolean set to true if the LUTs must be scaled on each timestep

    :param enable_live_viz: boolean set to true if the generated script should handle live-visualization.

    :param live_viz_frequency: integer specifying how often should the coprocessor send the live data

    :param cinema_tracks: cinema offline visualizer parameters

    :param cinema_arrays: selected value arrays for cinema

    :param filename: if specified, the script is written to the file.
    """
    from paraview.servermanager import vtkSMProxyManager
    version_str = vtkSMProxyManager.GetParaViewSourceVersion()

    # TODO: Once we're starting work on the PV export dialog we will need to
    # fill in timeStepToStartOutputAt, forceOutputAtFirstCall and requestSpecificNames
    # from the export options
    timeStepToStartOutputAt=0
    forceOutputAtFirstCall=False
    requestSpecificNames=False

    pipeline_script = cpstate.DumpPipeline(\
      export_rendering, simulation_input_map, screenshot_info, cinema_tracks,\
      cinema_arrays, enable_live_viz, live_viz_frequency)
    script = __output_contents % (version_str, timeStepToStartOutputAt, forceOutputAtFirstCall,
                                  padding_amount, rescale_data_range, requestSpecificNames, pipeline_script,
                                  enable_live_viz, live_viz_frequency)
    if filename:
        outFile = open(filename, "w")
        outFile.write(script)
        outFile.close()
    return script

def run(filename=None):
    """Create a dummy pipeline and save the coprocessing state in the filename
    specified, if any, else dumps it out on stdout."""

    from paraview import simple, servermanager
    wavelet = simple.Wavelet(registrationName="Wavelet1")
    contour = simple.Contour()
    script = DumpCoProcessingScript(export_rendering=False,
        simulation_input_map={"Wavelet1" : "input"},
        screenshot_info={},
        padding_amount=0,
        rescale_data_range=True,
        enable_live_viz=True,
        live_viz_frequency=1,
        cinema_tracks={},
        cinema_arrays = {},
        filename=filename)
    if not filename:
        print ("# *** Generated Script Begin ***")
        print (script)
        print ("# *** Generated Script End ***")

if __name__ == "__main__":
    run()

# ---- end ----
