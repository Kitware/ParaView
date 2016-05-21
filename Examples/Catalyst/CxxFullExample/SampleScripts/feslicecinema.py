from paraview.simple import *
from paraview import coprocessing

# --------------------- Cinema exporter definition ---------------------

def CreateCinemaExporter(input, frequency):
  from paraview import simple
  from paraview import data_exploration as cinema

  class FullAnalysis(object):
    def __init__(self):
      self.center_of_rotation = [34.5, 32.45, 27.95]
      self.rotation_axis = [0.0, 0.0, 1.0]
      self.distance = 500.0
      self.exporters = []
      self.analysis = cinema.AnalysisManager( 'cinema', "Cinema", "Test various cinema exporter.")
      self.analysis.begin()
      min = 0.0
      max = 642.0
      self.lut = simple.GetColorTransferFunction(
        "velocity",
        RGBPoints=[min, 0.23, 0.299, 0.754, (min+max)*0.5, 0.865, 0.865, 0.865, max, 0.706, 0.016, 0.15])
      # ==
      self.createSliceExporter()
      self.createComposite()
      self.createImageResampler()
      self.simple360()

    def createSliceExporter(self):
      self.analysis.register_analysis(
        "slice",                            # id
        "Slice exploration",                # title
        "Perform 10 slice along X",         # description
        "{time}/{sliceColor}_{slicePosition}.jpg", # data structure
        cinema.SliceExplorer.get_data_type())
      nb_slices = 5
      colorByArray = { "velocity": { "lut": self.lut , "type": 'POINT_DATA'} }
      view = simple.CreateRenderView()

      fng = self.analysis.get_file_name_generator("slice")
      exporter = cinema.SliceExplorer(fng, view, input, colorByArray, nb_slices)
      exporter.set_analysis(self.analysis)
      self.exporters.append(exporter)

    def createComposite(self):
      try:
        simple.LoadDistributedPlugin("RGBZView", ns=globals())
        self.analysis.register_analysis(
          "composite",
          "Composite rendering",
          "Performing composite on contour",
          '{time}/{theta}/{phi}/{filename}', cinema.CompositeImageExporter.get_data_type())
        fng = self.analysis.get_file_name_generator("composite")

        # Create pipeline to compose
        color_type = [('POINT_DATA', "velocity")]
        luts = { "velocity": self.lut }
        filters = [ input ]
        filters_description = [ {'name': 'catalyst'} ]
        color_by = [ color_type ]

        # Data exploration ------------------------------------------------------------
        camera_handler = cinema.ThreeSixtyCameraHandler(fng, None, [ float(r) for r in range(0, 360, 72)], [ float(r) for r in range(-60, 61, 45)], self.center_of_rotation, self.rotation_axis, self.distance)
        exporter = cinema.CompositeImageExporter(fng, filters, color_by, luts, camera_handler, [400,400], filters_description, 0, 0)
        exporter.set_analysis(self.analysis)
        self.exporters.append(exporter)
      except:
        print "Skip RGBZView exporter"

    def createImageResampler(self):
      self.analysis.register_analysis(
        "interactive-prober",                          # id
        "Interactive prober",                          # title
        "Sample data in image stack for line probing", # description
        "{time}/{field}/{slice}.{format}",                    # data structure
        cinema.ImageResampler.get_data_type())
      fng = self.analysis.get_file_name_generator("interactive-prober")
      arrays = { "velocity" : self.lut }
      exporter = cinema.ImageResampler(fng, input, [50,50,50], arrays)
      self.exporters.append(exporter)

    def simple360(self):
      self.analysis.register_analysis(
          "360",                                  # id
          "rotation",                             # title
          "Perform 15 contour",                   # description
          "{time}/{theta}_{phi}.jpg", # data structure
          cinema.ThreeSixtyImageStackExporter.get_data_type())
      fng = self.analysis.get_file_name_generator("360")
      arrayName = ('POINT_DATA', 'velocity')
      view = simple.CreateRenderView()

      rep = simple.Show(input, view)
      rep.LookupTable = self.lut
      rep.ColorArrayName = arrayName

      exporter = cinema.ThreeSixtyImageStackExporter(fng, view, self.center_of_rotation, self.distance, self.rotation_axis, [20,45])
      self.exporters.append(exporter)

    def UpdatePipeline(self, time):
      if time % frequency != 0:
        return

      # Do the exploration work
      for exporter in self.exporters:
        exporter.UpdatePipeline(time)

    def Finalize(self):
      self.analysis.end()

  return FullAnalysis()

# ----------------------- CoProcessor definition -----------------------

def CreateCoProcessor():
  def _CreatePipeline(coprocessor, datadescription):
    class Pipeline:
      filename_3_pvtu = coprocessor.CreateProducer( datadescription, "input" )

      Slice1 = Slice( guiName="Slice1", Crinkleslice=0, SliceOffsetValues=[0.0], Triangulatetheslice=1, SliceType="Plane" )
      Slice1.SliceType.Offset = 0.0
      Slice1.SliceType.Origin = [34.5, 32.45, 27.95]
      Slice1.SliceType.Normal = [1.0, 0.0, 0.0]

      # create a new 'Parallel PolyData Writer'
      parallelPolyDataWriter1 = servermanager.writers.XMLPPolyDataWriter(Input=Slice1)

      # register the writer with coprocessor
      # and provide it with information such as the filename to use,
      # how frequently to write the data, etc.
      coprocessor.RegisterWriter(parallelPolyDataWriter1, filename='slice_%t.pvtp', freq=10)

      # create a new 'Parallel UnstructuredGrid Writer'
      unstructuredGridWriter1 = servermanager.writers.XMLPUnstructuredGridWriter(Input=filename_3_pvtu)

      # register the writer with coprocessor
      # and provide it with information such as the filename to use,
      # how frequently to write the data, etc.
      coprocessor.RegisterWriter(unstructuredGridWriter1, filename='fullgrid_%t.pvtu', freq=100)

      CreateCinemaExporter(filename_3_pvtu, 2)

    return Pipeline()

  class CoProcessor(coprocessing.CoProcessor):
    def CreatePipeline(self, datadescription):
      self.Pipeline = _CreatePipeline(self, datadescription)

  coprocessor = CoProcessor()
  freqs = {'input': [10, 100]}
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
