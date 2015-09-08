
try: paraview.simple
except: from paraview.simple import *

from paraview import coprocessing
import sys

# ----------------------- CoProcessor definition -----------------------
def CreateCoProcessor():
  def _CreatePipeline(coprocessor, datadescription):
    class Pipeline:
      #### disable automatic camera reset on 'Show'
      paraview.simple._DisableFirstRenderCameraReset()

      # Create a new 'Render View'
      renderView1 = CreateView('RenderView')
      renderView1.ViewSize = [441, 1029]
      renderView1.InteractionMode = '2D'
      renderView1.CenterOfRotation = [0.5, 0.5, 0.0]
      renderView1.CameraPosition = [0.6346870059402945, 1.580702880997128, 10000.0]
      renderView1.CameraFocalPoint = [0.6346870059402945, 1.580702880997128, 0.0]
      renderView1.CameraParallelScale = 1.649915822768611
      renderView1.Background = [0.32, 0.34, 0.43]

      # register the view with coprocessor
      # and provide it with information such as the filename to use,
      # how frequently to write the images, etc.
      coprocessor.RegisterView(renderView1,
                               filename='results/image_%t.png', freq=1, fittoscreen=1, magnification=1, width=400, height=400)

      # ----------------------------------------------------------------
      # setup the data processing pipelines
      # ----------------------------------------------------------------

      Source = coprocessor.CreateProducer( datadescription, "input" )
      threshold1 = Threshold( Source )
      threshold1.Scalars = ['POINTS', 'Pressure']
      threshold1.ThresholdRange = [0.0, 0.5]
      # ----------------------------------------------------------------
      # setup color maps and opacity mapes used in the visualization
      # note: the Get..() functions create a new object, if needed
      # ----------------------------------------------------------------

      # get color transfer function/color map for 'Pressure'
      pressureLUT = GetColorTransferFunction('Pressure')
      pressureLUT.RGBPoints = [0.0, 0.231373, 0.298039, 0.752941, 0.10473327284884701, 0.865003, 0.865003, 0.865003, 0.20946654569769402, 0.705882, 0.0156863, 0.14902]
      pressureLUT.ScalarRangeInitialized = 1.0

      # get opacity transfer function/opacity map for 'Pressure'
      pressurePWF = GetOpacityTransferFunction('Pressure')
      pressurePWF.Points = [0.0, 0.0, 0.5, 0.0, 0.20946654569769402, 1.0, 0.5, 0.0]
      pressurePWF.ScalarRangeInitialized = 1

      # ----------------------------------------------------------------
      # setup the visualization in view 'renderView1'
      # ----------------------------------------------------------------

      # show data from Source
      SourceDisplay = Show(Source, renderView1)
      # trace defaults for the display properties.
      SourceDisplay.ColorArrayName = ['POINTS', 'Pressure']
      SourceDisplay.LookupTable = pressureLUT
      SourceDisplay.ScalarOpacityUnitDistance = 0.2349792427843548

      # show color legend
      SourceDisplay.SetScalarBarVisibility(renderView1, True)

      # setup the color legend parameters for each legend in this view

      # get color legend/bar for pressureLUT in view renderView1
      pressureLUTColorBar = GetScalarBar(pressureLUT, renderView1)
      pressureLUTColorBar.Title = 'Pressure'
      pressureLUTColorBar.ComponentTitle = ''

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
coprocessor.EnableLiveVisualization(True)

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
    print "Timestep:", timestep, "Time:", datadescription.GetTime()

    # Update the coprocessor by providing it the newly generated simulation data.
    # If the pipeline hasn't been setup yet, this will setup the pipeline.
    coprocessor.UpdateProducers(datadescription)
    coprocessor.WriteData(datadescription)
    coprocessor.WriteImages(datadescription)
    coprocessor.DoLiveVisualization(datadescription,"localhost",22222)
