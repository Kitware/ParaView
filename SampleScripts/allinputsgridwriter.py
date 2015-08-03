from paraview.simple import *

from paraview import coprocessing

# the frequency to output everything
outputfrequency = 5

# the name of the inputs that the adaptor provides
# for most it will just be 'input' by convention
namedinputs = ['input']

# ----------------------- CoProcessor definition -----------------------

def CreateCoProcessor():
  def _CreatePipeline(coprocessor, datadescription):
    class Pipeline:
      for name in namedinputs:
        inputdescription = datadescription.GetInputDescriptionByName(name)
        adaptorinput = coprocessor.CreateProducer( datadescription, name )
        grid = adaptorinput.GetClientSideObject().GetOutputDataObject(0)
        if  grid.IsA('vtkImageData') or grid.IsA('vtkUniformGrid'):
          writer = coprocessor.CreateWriter( XMLPImageDataWriter, name+"_%t.pvti", outputfrequency )
        elif  grid.IsA('vtkRectilinearGrid'):
          writer = coprocessor.CreateWriter( XMLPRectilinearGridWriter, name+"_%t.pvtr", outputfrequency )
        elif  grid.IsA('vtkStructuredGrid'):
          writer = coprocessor.CreateWriter( XMLPStructuredGridWriter, name+"_%t.pvts", outputfrequency )
        elif  grid.IsA('vtkPolyData'):
          writer = coprocessor.CreateWriter( XMLPPolyDataWriter, name+"_%t.pvtp", outputfrequency )
        elif  grid.IsA('vtkUnstructuredGrid'):
          writer = coprocessor.CreateWriter( XMLPUnstructuredGridWriter, name+"_%t.pvtu", outputfrequency )
        elif  grid.IsA('vtkUniformGridAMR'):
          writer = coprocessor.CreateWriter( XMLHierarchicalBoxDataWriter, name+"_%t.vthb", outputfrequency )
        elif  grid.IsA('vtkMultiBlockDataSet'):
          writer = coprocessor.CreateWriter( XMLMultiBlockDataWriter, name+"_%t.vtm", outputfrequency )
        else:
          print "Don't know how to create a writer for a ", grid.GetClassName()

    return Pipeline()

  class CoProcessor(coprocessing.CoProcessor):
    def CreatePipeline(self, datadescription):
      self.Pipeline = _CreatePipeline(self, datadescription)

  coprocessor = CoProcessor()
  freqs = {}
  for name in namedinputs:
    freqs[name] = [outputfrequency]

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
coprocessor.EnableLiveVisualization(False)


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
    coprocessor.UpdateProducers(datadescription)

    # Write output data, if appropriate.
    coprocessor.WriteData(datadescription);

    # Write image capture (Last arg: rescale lookup table), if appropriate.
    coprocessor.WriteImages(datadescription, rescale_lookuptable=False)

    # Live Visualization, if enabled.
    coprocessor.DoLiveVisualization(datadescription, "localhost", 22222)
