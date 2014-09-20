r"""
This module is designed for use in co-processing Python scripts. It provides a
class, Pipeline, which is designed to be used as the base-class for Python
pipeline. Additionally, this module has several other utility functions that are
approriate for co-processing.
"""

from paraview import simple, servermanager
from vtkPVVTKExtensionsCorePython import *
import math

# -----------------------------------------------------------------------------
def IsInModulo(timestep, frequencyArray):
    """
    Return True if the given timestep is in one of the provided frequency.
    This can be interpreted as follow::

        isFM = IsInModulo(timestep, [2,3,7])

    is similar to::

        isFM = (timestep % 2 == 0) or (timestep % 3 == 0) or (timestep % 7 == 0)
    """
    for frequency in frequencyArray:
        if frequency > 0 and (timestep % frequency == 0):
            return True
    return False

class CoProcessor(object):
    """Base class for co-processing Pipelines. paraview.cpstate Module can
       be used to dump out ParaView states as co-processing pipelines. Those are
       typically subclasses of this. The subclasses must provide an
       implementation for the CreatePipeline() method."""

    def __init__(self):
        self.__PipelineCreated = False
        self.__ProducersMap = {}
        self.__WritersList = []
        self.__ExporterList = []
        self.__ViewsList = []
        self.__EnableLiveVisualization = False
        self.__LiveVisualizationFrequency = 1;
        self.__LiveVisualizationLink = None
        pass

    def SetUpdateFrequencies(self, frequencies):
        """Set the frequencies at which the pipeline needs to be updated.
           Typically, this is called by the subclass once it has determined what
           timesteps co-processing will be needed to be done.
           frequencies is a map, with key->string name of for the simulation
           input, and value is a list of frequencies.
           """
        if type(frequencies) != dict:
           raise RuntimeError,\
                 "Incorrect argument type: %s, must be a dict" % type(frequencies)
        self.__Frequencies = frequencies


    def EnableLiveVisualization(self, enable, frequency = 1):
        """Call this method to enable live-visualization. When enabled,
        DoLiveVisualization() will communicate with ParaView server if possible
        for live visualization. Frequency specifies how often the
        communication happens (default is every second)."""
        self.__EnableLiveVisualization = enable
        self.__LiveVisualizationFrequency = frequency
        if (enable):
            for currentFrequencies in self.__Frequencies.itervalues():
                if not frequency in currentFrequencies:
                    currentFrequencies.append(frequency)
                    currentFrequencies.sort()

    def CreatePipeline(self, datadescription):
        """This methods must be overridden by subclasses to create the
           visualization pipeline."""
        raise RuntimeError, "Subclasses must override this method."

    def LoadRequestedData(self, datadescription):
        """Call this method in RequestDataDescription co-processing pass to mark
           the datadescription with information about what fields and grids are
           required for this pipeline for the given timestep, if any.

           Default implementation uses the update-frequencies set using
           SetUpdateFrequencies() to determine if the current timestep needs to
           be processed and then requests all fields. Subclasses can override
           this method to provide addtional customizations."""

        timestep = datadescription.GetTimeStep()
        num_inputs = datadescription.GetNumberOfInputDescriptions()
        for cc in range(num_inputs):
            input_name = datadescription.GetInputDescriptionName(cc)

            freqs = self.__Frequencies.get(input_name, [])
            if freqs:
                if IsInModulo(timestep, freqs) :
                    datadescription.GetInputDescription(cc).AllFieldsOn()
                    datadescription.GetInputDescription(cc).GenerateMeshOn()

    def UpdateProducers(self, datadescription):
        """This method will update the producers in the pipeline. If the
           pipeline is not created, it will be created using
           self.CreatePipeline().
        """
        if not self.__PipelineCreated:
           self.CreatePipeline(datadescription)
           self.__PipelineCreated = True

        simtime = datadescription.GetTime()
        for name, producer in self.__ProducersMap.iteritems():
            producer.GetClientSideObject().SetOutput(\
                datadescription.GetInputDescriptionByName(name).GetGrid(),
                simtime)

    def WriteData(self, datadescription):
        """This method will update all writes present in the pipeline, as
           needed, to generate the output data files, respecting the
           write-frequencies set on the writers."""
        timestep = datadescription.GetTimeStep()
        for writer in self.__WritersList:
            frequency = writer.parameters.GetProperty(
                "WriteFrequency").GetElement(0)
            fileName = writer.parameters.GetProperty("FileName").GetElement(0)
            if (timestep % frequency) == 0 or \
                    datadescription.GetForceOutput() == True:
                writer.FileName = fileName.replace("%t", str(timestep))
                writer.UpdatePipeline(datadescription.GetTime())

        for exporter in self.__ExporterList:
            exporter.UpdatePipeline(datadescription.GetTime())

    def WriteImages(self, datadescription, rescale_lookuptable=False):
        """This method will update all views, if present and write output
            images, as needed."""
        timestep = datadescription.GetTimeStep()

        for view in self.__ViewsList:
            if (view.cpFrequency and timestep % view.cpFrequency == 0) or \
               datadescription.GetForceOutput() == True:
                fname = view.cpFileName
                fname = fname.replace("%t", str(timestep))
                if view.cpFitToScreen != 0:
                    if view.IsA("vtkSMRenderViewProxy") == True:
                        view.ResetCamera()
                    elif view.IsA("vtkSMContextViewProxy") == True:
                        view.ResetDisplay()
                    else:
                        print ' do not know what to do with a ', view.GetClassName()
                view.ViewTime = datadescription.GetTime()
                if rescale_lookuptable:
                    self.RescaleDataRange(view, datadescription.GetTime())
                simple.WriteImage(fname, view, Magnification=view.cpMagnification)

    def DoLiveVisualization(self, datadescription, hostname, port):
        """This method execute the code-stub needed to communicate with ParaView
           for live-visualization. Call this method only if you want to support
           live-visualization with your co-processing module."""

        if (not self.__EnableLiveVisualization or
            (datadescription.GetTimeStep() %
             self.__LiveVisualizationFrequency) != 0):
            return

        # make sure the live insitu is initialized
        if not self.__LiveVisualizationLink:
           # Create the vtkLiveInsituLink i.e.  the "link" to the visualization processes.
           self.__LiveVisualizationLink = servermanager.vtkLiveInsituLink()

           # Tell vtkLiveInsituLink what host/port must it connect to
           # for the visualization process.
           self.__LiveVisualizationLink.SetHostname(hostname)
           self.__LiveVisualizationLink.SetInsituPort(int(port))

           # Initialize the "link"
           self.__LiveVisualizationLink.InsituInitialize(servermanager.ActiveConnection.Session.GetSessionProxyManager())

        time = datadescription.GetTime()
        timeStep = datadescription.GetTimeStep()

        # stay in the loop while the simulation is paused
        while True:
            # Update the simulation state, extracts and simulationPaused
            # from ParaView Live
            self.__LiveVisualizationLink.InsituUpdate(time, timeStep)

            # sources need to be updated by insitu
            # code. vtkLiveInsituLink never updates the pipeline, it
            # simply uses the data available at the end of the
            # pipeline, if any.
            from paraview import simple
            for source in simple.GetSources().values():
                source.UpdatePipeline(time)

            # push extracts to the visualization process.
            self.__LiveVisualizationLink.InsituPostProcess(time, timeStep)

            if (self.__LiveVisualizationLink.GetSimulationPaused()):
                # This blocks until something changes on ParaView Live
                # and then it continues the loop. Returns != 0 if LIVE side
                # disconnects
                if (self.__LiveVisualizationLink.WaitForLiveChange()):
                    break;
            else:
                break


    def CreateProducer(self, datadescription, inputname):
        """Creates a producer proxy for the grid. This method is generally used in
         CreatePipeline() call to create producers."""
        # Check that the producer name for the input given is valid for the
        # current setup.
        if not datadescription.GetInputDescriptionByName(inputname):
            raise RuntimeError, "Simulation input name '%s' does not exist" % inputname

        grid = datadescription.GetInputDescriptionByName(inputname).GetGrid()

        producer = simple.PVTrivialProducer()
        # we purposefully don't set the time for the PVTrivialProducer here.
        # when we update the pipeline we will do it then.
        producer.GetClientSideObject().SetOutput(grid)

        if grid.IsA("vtkImageData") == True or \
                grid.IsA("vtkStructuredGrid") == True or \
                grid.IsA("vtkRectilinearGrid") == True:
            extent = datadescription.GetInputDescriptionByName(inputname).GetWholeExtent()
            producer.WholeExtent= [ extent[0], extent[1], extent[2], extent[3], extent[4], extent[5] ]

        # Save the producer for easy access in UpdateProducers() call.
        self.__ProducersMap[inputname] = producer
        producer.UpdatePipeline()
        return producer

    def RegisterExporter(self, exporter):
        """
        Registers a python object that will be responsible to export any
        kind of data. That exporter needs to provide the following set of
        methods:
            UpdatePipeline(time)
            Finalize()

        It will be the responsability of the exporter to skip timestep inside
        the UpdatePipeline(time) method when the given time does not match the
        targetted frequency.

        The coprocessing engine will automatically call UpdatePipeline(time)
        for each timestep on each registered exporter.
        Once the simulation is done, the Finalize() method will then be called
        to all exporter.
        """
        self.__ExporterList.append(exporter)

    def RegisterWriter(self, writer, filename, freq):
        """Registers a writer proxy. This method is generally used in
           CreatePipeline() to register writers. All writes created as such will
           write the output files appropriately in WriteData() is called."""
        writerParametersProxy = self.WriterParametersProxy(
            writer, filename, freq)

        writer.FileName = filename
        writer.add_attribute("parameters", writerParametersProxy)
        self.__WritersList.append(writer)

        return writer

    def WriterParametersProxy(self, writer, filename, freq):
        """Creates a client only proxy that will be synchronized with ParaView
        Live, allowing a user to set the filename and frequency.
        """
        controller = servermanager.ParaViewPipelineController()
        # assume that a client only proxy with the same name as a writer
        # is available in "insitu_writer_paramters"

        # Since coprocessor sometimes pass writer as a custom object and not
        # a proxy, we need to handle that. Just creating any arbitrary writer
        # proxy to store the parameters it acceptable. So let's just do that
        # when the writer is not a proxy.
        writerIsProxy = isinstance(writer, servermanager.Proxy)
        helperName = writer.GetXMLName() if writerIsProxy else "XMLPImageDataWriter"
        proxy = servermanager.ProxyManager().NewProxy(
            "insitu_writer_parameters", helperName)
        controller.PreInitializeProxy(proxy)
        if writerIsProxy:
            proxy.GetProperty("Input").SetInputConnection(
                0, writer.Input.SMProxy, 0)
        proxy.GetProperty("FileName").SetElement(0, filename)
        proxy.GetProperty("WriteFrequency").SetElement(0, freq)
        controller.PostInitializeProxy(proxy)
        controller.RegisterPipelineProxy(proxy)
        return proxy

    def RegisterView(self, view, filename, freq, fittoscreen, magnification, width, height):
        """Register a view for image capture with extra meta-data such
        as magnification, size and frequency."""
        if not isinstance(view, servermanager.Proxy):
            raise RuntimeError, "Invalid 'view' argument passed to RegisterView."
        view.add_attribute("cpFileName", filename)
        view.add_attribute("cpFrequency", freq)
        view.add_attribute("cpFileName", filename)
        view.add_attribute("cpFitToScreen", fittoscreen)
        view.add_attribute("cpMagnification", magnification)
        view.ViewSize = [ width, height ]
        self.__ViewsList.append(view)
        return view

    def CreateWriter(self, proxy_ctor, filename, freq):
        """ **** DEPRECATED!!! Use RegisterWriter instead ****
           Creates a writer proxy. This method is generally used in
           CreatePipeline() to create writers. All writes created as such will
           write the output files appropriately in WriteData() is called."""
        writer = proxy_ctor()
        return self.RegisterWriter(writer, filename, freq)

    def CreateView(self, proxy_ctor, filename, freq, fittoscreen, magnification, width, height):
        """ **** DEPRECATED!!! Use RegisterView instead ****
           Create a CoProcessing view for image capture with extra meta-data
           such as magnification, size and frequency."""
        view = proxy_ctor()
        return self.RegisterView(view, filename, freq, fittoscreen, magnification, width, height)

    def Finalize(self):
        for writer in self.__WritersList:
            if hasattr(writer, 'Finalize'):
                writer.Finalize()
        for view in self.__ViewsList:
            if hasattr(view, 'Finalize'):
                view.Finalize()
        for exporter in self.__ExporterList:
            if hasattr(view, 'Finalize'):
                exporter.Finalize()

    def RescaleDataRange(self, view, time):
        """DataRange can change across time, sometime we want to rescale the
           color map to match to the closer actual data range."""
        reps = view.Representations
        for rep in reps:
            if not hasattr(rep, 'Visibility') or \
                not rep.Visibility or \
                not hasattr(rep, 'MapScalars') or \
                not rep.MapScalars or \
                not rep.LookupTable:
                # rep is either not visibile or not mapping scalars using a LUT.
                continue;

            input = rep.Input
            input.UpdatePipeline(time) #make sure range is up-to-date
            lut = rep.LookupTable
            if rep.ColorAttributeType == 'POINT_DATA':
                datainformation = input.GetPointDataInformation()
            elif rep.ColorAttributeType == 'CELL_DATA':
                datainformation = input.GetCellDataInformation()
            else:
                print 'something strange with color attribute type', rep.ColorAttributeType

            if datainformation.GetArray(rep.ColorArrayName) == None:
                # there is no array on this process. it's possible
                # that this process has no points or cells
                continue

            if lut.VectorMode != 'Magnitude' or \
               datainformation.GetArray(rep.ColorArrayName).GetNumberOfComponents() == 1:
                datarange = datainformation.GetArray(rep.ColorArrayName).GetRange(lut.VectorComponent)
            else:
                # -1 corresponds to the magnitude.
                datarange = datainformation.GetArray(rep.ColorArrayName).GetRange(-1)

            import vtkParallelCorePython
            import paraview.vtk as vtk
            import paraview.servermanager
            pm = paraview.servermanager.vtkProcessModule.GetProcessModule()
            globalController = pm.GetGlobalController()
            localarray = vtk.vtkDoubleArray()
            localarray.SetNumberOfTuples(2)
            localarray.SetValue(0, -datarange[0]) # negate so that MPI_MAX gets min instead of doing a MPI_MIN and MPI_MAX
            localarray.SetValue(1, datarange[1])
            globalarray = vtk.vtkDoubleArray()
            globalarray.SetNumberOfTuples(2)
            globalController.AllReduce(localarray, globalarray, 0)
            globaldatarange = [-globalarray.GetValue(0), globalarray.GetValue(1)]
            rgbpoints = lut.RGBPoints.GetData()
            numpts = len(rgbpoints)/4
            if globaldatarange[0] != rgbpoints[0] or globaldatarange[1] != rgbpoints[(numpts-1)*4]:
                # rescale all of the points
                oldrange = rgbpoints[(numpts-1)*4] - rgbpoints[0]
                newrange = globaldatarange[1] - globaldatarange[0]
                # only readjust if the new range isn't zero.
                if newrange != 0:
                   newrgbpoints = list(rgbpoints)
                   # if the old range isn't 0 then we use that ranges distribution
                   if oldrange != 0:
                      for v in range(numpts-1):
                         newrgbpoints[v*4] = globaldatarange[0]+(rgbpoints[v*4] - rgbpoints[0])*newrange/oldrange

                      # avoid numerical round-off, at least with the last point
                      newrgbpoints[(numpts-1)*4] = globaldatarange[1]
                   else: # the old range is 0 so the best we can do is to space the new points evenly
                      for v in range(numpts+1):
                         newrgbpoints[v*4] = globaldatarange[0]+v*newrange/(1.0*numpts)

                   lut.RGBPoints.SetData(newrgbpoints)
