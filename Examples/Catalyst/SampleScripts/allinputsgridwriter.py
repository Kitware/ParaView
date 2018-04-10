from paraview.simple import *

from paraview import coprocessing

# the frequency to output everything
outputfrequency = 5

# ----------------------- CoProcessor definition -----------------------

def CreateCoProcessor():
  def _CreatePipeline(coprocessor, datadescription):
    class Pipeline:
      for i in range(datadescription.GetNumberOfInputDescriptions()):
        inputdescription = datadescription.GetInputDescription(i)
        name = datadescription.GetInputDescriptionName(i)
        adaptorinput = coprocessor.CreateProducer( datadescription, name )
        grid = adaptorinput.GetClientSideObject().GetOutputDataObject(0)
        extension = None
        if  grid.IsA('vtkImageData') or grid.IsA('vtkUniformGrid'):
          writer = servermanager.writers.XMLPImageDataWriter(Input=adaptorinput)
          extension = '.pvti'
        elif  grid.IsA('vtkRectilinearGrid'):
          writer = servermanager.writers.XMLPRectilinearGridWriter(Input=adaptorinput)
          extension = '.pvtr'
        elif  grid.IsA('vtkStructuredGrid'):
          writer = servermanager.writers.XMLPStructuredGridWriter(Input=adaptorinput)
          extension = '.pvts'
        elif  grid.IsA('vtkPolyData'):
          writer = servermanager.writers.XMLPPolyDataWriter(Input=adaptorinput)
          extension = '.pvtp'
        elif  grid.IsA('vtkUnstructuredGrid'):
          writer = servermanager.writers.XMLPUnstructuredGridWriter(Input=adaptorinput)
          extension = '.pvtu'
        elif  grid.IsA('vtkUniformGridAMR'):
          writer = servermanager.writers.XMLHierarchicalBoxDataWriter(Input=adaptorinput)
          extension = '.vthb'
        elif  grid.IsA('vtkMultiBlockDataSet'):
          writer = servermanager.writers.XMLMultiBlockDataWriter(Input=adaptorinput)
          extension = '.vtm'
        elif  grid.IsA('vtkHyperTreeGrid'):
          writer = servermanager.writers.HyperTreeGridWriter(Input=adaptorinput)
          extension = '.htg'
        else:
          print("Don't know how to create a writer for a ", grid.GetClassName())

        if extension:
          name = name.translate(None, '/') # Get rid of any slashes in the channel name
          coprocessor.RegisterWriter(writer, filename=name+'_%t'+extension, freq=outputfrequency)

    return Pipeline()

  class CoProcessor(coprocessing.CoProcessor):
    def CreatePipeline(self, datadescription):
      self.Pipeline = _CreatePipeline(self, datadescription)

  return CoProcessor()

#--------------------------------------------------------------
# Global variables that will hold the pipeline for each timestep
# Creating the CoProcessor object, doesn't actually create the ParaView pipeline.
# It will be automatically setup when coprocessor.UpdateProducers() is called the
# first time.
coprocessor = CreateCoProcessor()

#--------------------------------------------------------------
# Enable Live-Visualizaton with ParaView
coprocessor.EnableLiveVisualization(False)


# ---------------------- Data Selection method ----------------------

def RequestDataDescription(datadescription):
    "Callback to populate the request for current timestep"
    global coprocessor
    if datadescription.GetForceOutput() == True or datadescription.GetTimeStep() % outputfrequency == 0:
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

    # Write output data, if appropriate.
    coprocessor.WriteData(datadescription);

    # Write image capture (Last arg: rescale lookup table), if appropriate.
    coprocessor.WriteImages(datadescription, rescale_lookuptable=False)

    # Live Visualization, if enabled.
    coprocessor.DoLiveVisualization(datadescription, "localhost", 22222)
