from paraview.simple import *
from paraview import coprocessing

#--------------------------------------------------------------
# Code generated from cpstate.py to create the CoProcessor.
# ParaView 4.2.0-RC1-22-g9ca4e44 64 bits

# ----------------------- CoProcessor definition -----------------------

def CreateCoProcessor():
  def _CreatePipeline(coprocessor, datadescription):
    class Pipeline:
      # state file generated using paraview version 4.2.0-RC1-22-g9ca4e44

      # ----------------------------------------------------------------
      # setup the data processing pipelines
      # ----------------------------------------------------------------

      #### disable automatic camera reset on 'Show'
      paraview.simple._DisableFirstRenderCameraReset()

      # create a new 'XML Partitioned Unstructured Grid Reader'
      # create a producer from a simulation input
      outputcp_98pvtu = coprocessor.CreateProducer(datadescription, 'input')

      # create a new 'Programmable Filter'
      programmableFilter1 = ProgrammableFilter(Input=outputcp_98pvtu)
      programmableFilter1.Script = 'pdi = self.GetInput()\nnumPts = pdi.GetNumberOfPoints()\nca = vtk.vtkFloatArray()\nca.SetName("dogs")\nca.SetNumberOfComponents(1)\nca.SetNumberOfTuples(numPts)\nfor i in range(numPts):\n  ca.SetValue(i, 100)\n\nido = self.GetOutput()\nido.GetPointData().AddArray(ca)'
      programmableFilter1.RequestInformationScript = ''
      programmableFilter1.RequestUpdateExtentScript = ''
      programmableFilter1.PythonPath = ''

      # create a new 'Parallel UnstructuredGrid Writer'
      #parallelUnstructuredGridWriter1 = servermanager.writers.XMLPUnstructuredGridWriter(Input=programmableFilter1)

      # register the writer with coprocessor
      # and provide it with information such as the filename to use,
      # how frequently to write the data, etc.
      #coprocessor.RegisterWriter(parallelUnstructuredGridWriter1, filename='cppf_%t.pvtu', freq=10)

    return Pipeline()

  class CoProcessor(coprocessing.CoProcessor):
    def CreatePipeline(self, datadescription):
      self.Pipeline = _CreatePipeline(self, datadescription)

  coprocessor = CoProcessor()
  # these are the frequencies at which the coprocessor updates.
  freqs = {'input': [10]}
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

    print("in DoCoProcessing")

    # Update the coprocessor by providing it the newly generated simulation data.
    # If the pipeline hasn't been setup yet, this will setup the pipeline.
    coprocessor.UpdateProducers(datadescription)

    # Write output data, if appropriate.
    coprocessor.WriteData(datadescription);

    # Write image capture (Last arg: rescale lookup table), if appropriate.
    coprocessor.WriteImages(datadescription, rescale_lookuptable=False)

    # Live Visualization, if enabled.
    coprocessor.DoLiveVisualization(datadescription, "localhost", 22222)
