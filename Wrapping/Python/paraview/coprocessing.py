r"""
This module is designed for use in co-processing Python scripts. It provides a
class, Pipeline, which is designed to be used as the base-class for Python
pipeline. Additionally, this module has several other utility functions that are
appropriate for co-processing.
"""

# for Python2 print statmements to output like Python3 print statements
from __future__ import print_function
from paraview import simple, servermanager
from paraview.modules.vtkPVVTKExtensionsCore import *
from paraview.detail import exportnow

import math

# If the user created a filename in a location that doesn't exist by default we'll
# make the directory for them. This can be changed though by setting createDirectoriesIfNeeded
# to False.
createDirectoriesIfNeeded = True

# -----------------------------------------------------------------------------

class CoProcessor(object):
    """Base class for co-processing Pipelines.

    paraview.cpstate Module can be used to dump out ParaView states as
    co-processing pipelines. Those are typically subclasses of this. The
    subclasses must provide an implementation for the CreatePipeline() method.

    **Cinema Tracks**

    CoProcessor maintains user-defined information for the Cinema generation in
    __CinemaTracks. This information includes track parameter values, data array
    names, etc. __CinemaTracks holds this information in the following structure::

        {
          proxy_reference : {
          'ControlName' : [value_1, value_2, ..., value_n],
          'arraySelection' : ['ArrayName_1', ..., 'ArrayName_n']
          }
        }

    __CinemaTracks is populated when defining the co-processing pipline through
    paraview.cpstate. paraview.cpstate uses accessor instances to set values and
    array names through the RegisterCinemaTrack and AddArrayssToCinemaTrack
    methods of this class.
    """

    def __init__(self):
        self.__PipelineCreated = False
        # __ProducersMap will have a list of the producers/channels that need to be
        # created by the adaptor for this pipeline. The adaptor may be able to generate
        # other channels as well though.
        self.__ProducersMap = {}
        self.__WritersList = []
        self.__ViewsList = []
        self.__EnableLiveVisualization = False
        self.__LiveVisualizationFrequency = 1;
        self.__LiveVisualizationLink = None
        # __CinemaTracksList is just for Spec-A compatibility (will be deprecated
        # when porting Spec-A to pv_introspect. Use __CinemaTracks instead.
        self.__CinemaTracksList = []
        self.__CinemaTracks = {}
        self.__InitialFrequencies = {}
        self.__PrintEnsightFormatString = False
        self.__TimeStepToStartOutputAt=0
        self.__ForceOutputAtFirstCall=False
        self.__FirstTimeStepIndex = None
        # a list of arrays requested for each channel, e.g. {'input': ["a point data array name", 0], ["a cell data array name", 1]}
        self.__RequestedArrays = None
        self.__RootDirectory = ""
        self.__CinemaDHelper = None

    def SetPrintEnsightFormatString(self, enable):
        """If outputting ExodusII files with the purpose of reading them into
           Ensight, print a message on process 0 on what to use for the 'Set string'
           input to properly read the generated files into Ensight."""
        self.__PrintEnsightFormatString = enable

    def SetUpdateFrequencies(self, frequencies):
        """Set the frequencies at which the pipeline needs to be updated.
           Typically, this is called by the subclass once it has determined what
           timesteps co-processing will be needed to be done.
           frequencies is a map, with key->string name of for the simulation
           input, and value is a list of frequencies.
           """
        if type(frequencies) != dict:
           raise RuntimeError (
                 "Incorrect argument type: %s, must be a dict" % type(frequencies))
        self.__InitialFrequencies = frequencies

    def SetRequestedArrays(self, channelname, requestedarrays):
        """Set which arrays this script will request from the adaptor for the given channel name.
        """
        if not self.__RequestedArrays:
            self.__RequestedArrays = {}
        self.__RequestedArrays[channelname] = requestedarrays

    def SetInitialOutputOptions(self, timeStepToStartOutputAt, forceOutputAtFirstCall):
        """Set the frequencies at which the pipeline needs to be updated.
           Typically, this is called by the subclass once it has determined what
           timesteps co-processing will be needed to be done.
           frequencies is a map, with key->string name of for the simulation
           input, and value is a list of frequencies.
           """

        self.__TimeStepToStartOutputAt=timeStepToStartOutputAt
        self.__ForceOutputAtFirstCall=forceOutputAtFirstCall

    def EnableLiveVisualization(self, enable, frequency = 1):
        """Call this method to enable live-visualization. When enabled,
        DoLiveVisualization() will communicate with ParaView server if possible
        for live visualization. Frequency specifies how often the
        communication happens (default is every second)."""
        self.__EnableLiveVisualization = enable
        self.__LiveVisualizationFrequency = frequency

    def CreatePipeline(self, datadescription):
        """This methods must be overridden by subclasses to create the
           visualization pipeline."""
        raise RuntimeError ("Subclasses must override this method.")

    def LoadRequestedData(self, datadescription):
        """Call this method in RequestDataDescription co-processing pass to mark
           the datadescription with information about what fields and grids are
           required for this pipeline for the given timestep, if any.

           Default implementation uses the update-frequencies set using
           SetUpdateFrequencies() to determine if the current timestep needs to
           be processed and then requests all fields. Subclasses can override
           this method to provide additional customizations. If there is a Live
           connection that can also override the initial frequencies."""

        # if this is a time step to do live then only the channels that were requested when
        # generating the script will be made available even though the adaptor may be able
        # to provide other channels. similarly, if only specific arrays were requested when
        # generating the script then only those arrays will be provided to the Live connection.
        # note that we want the pipeline built before we do the actual first live connection.
        if self.__EnableLiveVisualization and self.NeedToOutput(datadescription, self.__LiveVisualizationFrequency) \
           and self.__LiveVisualizationLink:
            if self.__LiveVisualizationLink.Initialize(servermanager.ActiveConnection.Session.GetSessionProxyManager()):
                if self.__RequestedArrays:
                    for key in self.__RequestedArrays:
                        for v in self.__RequestedArrays[key]:
                            datadescription.GetInputDescriptionByName(key).AddField(v[0], v[1])
                elif self.__InitialFrequencies:
                    # __ProducersMap will not be filled up until after the first call to
                    # DoCoProcessing so we rely on __InitialFrequencies initially but then
                    # __ProducersMap after that as __InitialFrequencies will be cleared out.
                    for key in self.__InitialFrequencies:
                        datadescription.GetInputDescriptionByName(key).AllFieldsOn()
                        datadescription.GetInputDescriptionByName(key).GenerateMeshOn()
                else:
                    for key in self.__ProducersMap:
                        datadescription.GetInputDescriptionByName(key).AllFieldsOn()
                        datadescription.GetInputDescriptionByName(key).GenerateMeshOn()
                return

        # if we haven't processed the pipeline yet in DoCoProcessing() we
        # must use the initial frequencies to figure out if there's
        # work to do this time/timestep. If we don't have Live enabled
        # we know that the output frequencies aren't changed and can
        # just use the initial frequencies.
        if self.__ForceOutputAtFirstCall or self.__InitialFrequencies or not self.__EnableLiveVisualization:
            if self.__RequestedArrays:
                for key in self.__RequestedArrays:
                    for v in self.__RequestedArrays[key]:
                        datadescription.GetInputDescriptionByName(key).AddField(v[0], v[1])
            elif self.__InitialFrequencies:
                for key in self.__InitialFrequencies:
                    freqs = self.__InitialFrequencies.get(key, [])
                    if self.__EnableLiveVisualization or self.IsInModulo(datadescription, freqs):
                        datadescription.GetInputDescriptionByName(key).AllFieldsOn()
                        datadescription.GetInputDescriptionByName(key).GenerateMeshOn()
        else:
            # the catalyst pipeline may have been changed by a Live connection
            # so we need to regenerate the frequencies
            from paraview import cpstate
            frequencies = {}
            for writer in self.__WritersList:
                frequency = writer.parameters.GetProperty("WriteFrequency").GetElement(0)
                if self.NeedToOutput(datadescription, frequency) or datadescription.GetForceOutput() == True:
                    writerinputs = cpstate.locate_simulation_inputs(writer)
                    for writerinput in writerinputs:
                        if self.__RequestedArrays:
                            for key in self.__RequestedArrays:
                                for v in self.__RequestedArrays[key]:
                                    datadescription.GetInputDescriptionByName(writerinput).AddField(v[0], v[1])
                        else:
                            datadescription.GetInputDescriptionByName(writerinput).AllFieldsOn()
                            datadescription.GetInputDescriptionByName(writerinput).GenerateMeshOn()

            for view in self.__ViewsList:
                if (view.cpFrequency and self.NeedToOutput(datadescription, view.cpFrequency)) or \
                   datadescription.GetForceOutput() == True:
                    viewinputs = cpstate.locate_simulation_inputs_for_view(view)
                    for viewinput in viewinputs:
                        if self.__RequestedArrays:
                            for key in self.__RequestedArrays:
                                for v in self.__RequestedArrays[key]:
                                    datadescription.GetInputDescriptionByName(viewinput).AddField(v[0], v[1])
                        else:
                            datadescription.GetInputDescriptionByName(viewinput).AllFieldsOn()
                            datadescription.GetInputDescriptionByName(viewinput).GenerateMeshOn()


    def UpdateProducers(self, datadescription):
        """This method will update the producers in the pipeline. If the
           pipeline is not created, it will be created using
           self.CreatePipeline().
        """
        if not self.__PipelineCreated:
           self.CreatePipeline(datadescription)
           self.__PipelineCreated = True
           if self.__EnableLiveVisualization:
               # we don't want to use __InitialFrequencies any more with live viz
               self.__InitialFrequencies = None
           self.__FixupWriters()

        else:
            simtime = datadescription.GetTime()
            for name, producer in self.__ProducersMap.items():
                producer.GetClientSideObject().SetOutput(
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
            if self.NeedToOutput(datadescription, frequency) or datadescription.GetForceOutput() == True:
                fileName = writer.parameters.GetProperty("FileName").GetElement(0)
                paddingamount = writer.parameters.GetProperty("PaddingAmount").GetElement(0)
                helperName = writer.GetXMLName()
                if helperName == "ExodusIIWriter":
                    ts = "."+str(timestep).rjust(paddingamount, '0')
                    writer.FileName = fileName + ts
                else:
                    ts = str(timestep).rjust(paddingamount, '0')
                    writer.FileName = fileName.replace("%t", ts)
                if '/' in writer.FileName and createDirectoriesIfNeeded:
                    oktowrite = [1.]
                    import vtk
                    comm = vtk.vtkMultiProcessController.GetGlobalController()
                    if comm.GetLocalProcessId() == 0:
                        import os
                        newDir = writer.FileName[0:writer.FileName.rfind('/')]
                        try:
                            os.makedirs(newDir)
                        except OSError:
                            if not os.path.isdir(newDir):
                                print ("ERROR: Cannot make directory for", writer.FileName, ". No data will be written.")
                                oktowrite[0] = 0.
                    comm.Broadcast(oktowrite, 1, 0)
                    if oktowrite[0] == 0:
                        # we can't make the directory so no reason to update the pipeline
                        return
                writer.UpdatePipeline(datadescription.GetTime())
                self.__AppendToCinemaDTable(timestep, "writer_%s" % self.__WritersList.index(writer), writer.FileName)
        self.__FinalizeCinemaDTable()


    def WriteImages(self, datadescription, rescale_lookuptable=False,
                    image_quality=None, padding_amount=0):
        """This method will update all views, if present and write output
        images, as needed.

        **Parameters**

            datadescription
              Catalyst data-description object

            rescale_lookuptable (bool, optional)
              If True, when all lookup tables
              are rescaled using current data ranges before saving the images.
              Defaults to False.

            image_quality (int, optional)
              If specified, should be a value in
              the range (0, 100) that specifies the image quality. For JPEG, 0
              is low quality i.e. max compression, 100 is best quality i.e.
              least compression. For legacy reasons, this is inverted for PNG
              (which uses lossless compression). For PNG, 0 is no compression
              i.e maximum image size, while 100 is most compressed and hence
              least image size.

              If not specified, for saving PNGs 0 is assumed to minimize
              performance impact.

            padding_amount (int, optional)
              Amount to pad the time index by.

        """
        timestep = datadescription.GetTimeStep()

        cinema_dirs = []
        for view in self.__ViewsList:
            if (view.cpFrequency and self.NeedToOutput(datadescription, view.cpFrequency)) or \
               datadescription.GetForceOutput() == True:
                fname = view.cpFileName
                ts = str(timestep).rjust(padding_amount, '0')
                fname = fname.replace("%t", ts)
                if view.cpFitToScreen != 0:
                    view.ViewTime = datadescription.GetTime()
                    if view.IsA("vtkSMRenderViewProxy") == True:
                        view.ResetCamera()
                    elif view.IsA("vtkSMContextViewProxy") == True:
                        view.ResetDisplay()
                    else:
                        print (' do not know what to do with a ', view.GetClassName())
                view.ViewTime = datadescription.GetTime()
                if rescale_lookuptable:
                    self.RescaleDataRange(view, datadescription.GetTime())
                cinemaOptions = view.cpCinemaOptions
                if cinemaOptions and 'camera' in cinemaOptions:
                    if 'composite' in view.cpCinemaOptions and view.cpCinemaOptions['composite'] == True:
                        dirname, filelist = self.UpdateCinema(view, datadescription,
                                                    specLevel="B")
                    else:
                        dirname, filelist = self.UpdateCinema(view, datadescription,
                                                    specLevel="A")
                    if dirname:
                        self.__AppendCViewToCinemaDTable(timestep, "view_%s" % self.__ViewsList.index(view), filelist)
                        cinema_dirs.append(dirname)
                else:
                    if '/' in fname and createDirectoriesIfNeeded:
                        oktowrite = [1.]
                        import vtk
                        comm = vtk.vtkMultiProcessController.GetGlobalController()
                        if comm.GetLocalProcessId() == 0:
                            import os
                            newDir = fname[0:fname.rfind('/')]
                            try:
                                os.makedirs(newDir)
                            except OSError:
                                if not os.path.isdir(newDir):
                                    print ("ERROR: Cannot make directory for", fname, ". No image will be output.")
                                    oktowrite[0] = 0.
                        comm.Broadcast(oktowrite, 1, 0)
                        if oktowrite[0] == 0:
                            # we can't make the directory so no reason to update the pipeline
                            return

                    if image_quality is None and fname.endswith('png'):
                        # for png quality = 0 means no compression. compression can be a potentially
                        # very costly serial operation on process 0
                        quality = 0
                    elif image_quality is not None:
                        quality = int(image_quality)
                    else:
                        # let simple.SaveScreenshot pick a default.
                        quality = None

                    if fname.endswith('png') and view.cpCompression is not None and view.cpCompression != -1 :
                        simple.SaveScreenshot(fname, view,
                                              CompressionLevel=view.cpCompression,
                                              ImageResolution=view.ViewSize)
                    else:
                        simple.SaveScreenshot(fname, view,
                                              magnification=view.cpMagnification,
                                              quality=quality)
                    self.__AppendToCinemaDTable(timestep, "view_%s" % self.__ViewsList.index(view), fname)

        if len(cinema_dirs) > 1:
            import paraview.tpl.cinema_python.adaptors.paraview.pv_introspect as pv_introspect
            pv_introspect.make_workspace_file("cinema\\", cinema_dirs)

        self.__FinalizeCinemaDTable()


    def DoLiveVisualization(self, datadescription, hostname, port):
        """This method execute the code-stub needed to communicate with ParaView
           for live-visualization. Call this method only if you want to support
           live-visualization with your co-processing module."""

        if not self.__EnableLiveVisualization:
            return

        if not self.__LiveVisualizationLink and self.__EnableLiveVisualization:
            # Create the vtkLiveInsituLink i.e.  the "link" to the visualization processes.
            self.__LiveVisualizationLink = servermanager.vtkLiveInsituLink()

            # Tell vtkLiveInsituLink what host/port must it connect to
            # for the visualization process.
            self.__LiveVisualizationLink.SetHostname(hostname)
            self.__LiveVisualizationLink.SetInsituPort(int(port))

            # Initialize the "link"
            self.__LiveVisualizationLink.Initialize(servermanager.ActiveConnection.Session.GetSessionProxyManager())


        if self.__EnableLiveVisualization and self.NeedToOutput(datadescription, self.__LiveVisualizationFrequency):
            if not self.__LiveVisualizationLink.Initialize(servermanager.ActiveConnection.Session.GetSessionProxyManager()):
                return

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
            raise RuntimeError ("Simulation input name '%s' does not exist" % inputname)

        grid = datadescription.GetInputDescriptionByName(inputname).GetGrid()
        if not grid:
            # we have a description of this channel but we don't need the grid so return
            return

        producer = simple.PVTrivialProducer(guiName=inputname)
        producer.add_attribute("cpSimulationInput", inputname)
        # mark this as an input proxy so we can use cpstate.locate_simulation_inputs()
        # to find it
        producer.SMProxy.cpSimulationInput = inputname

        # we purposefully don't set the time for the PVTrivialProducer here.
        # when we update the pipeline we will do it then.
        producer.GetClientSideObject().SetOutput(grid, datadescription.GetTime())

        if grid.IsA("vtkImageData") == True or \
                grid.IsA("vtkStructuredGrid") == True or \
                grid.IsA("vtkRectilinearGrid") == True:
            extent = datadescription.GetInputDescriptionByName(inputname).GetWholeExtent()
            producer.WholeExtent= [ extent[0], extent[1], extent[2], extent[3], extent[4], extent[5] ]

        # Save the producer for easy access in UpdateProducers() call.
        self.__ProducersMap[inputname] = producer
        producer.UpdatePipeline(datadescription.GetTime())
        return producer

    def ProcessExodusIIWriter(self, writer):
        """Extra work for the ExodusII writer to avoid undesired warnings
           and print out a message on how to read the files into Ensight."""
        # Disable the warning about not having meta data available since we can
        # use this writer for vtkDataSets
        writer.IgnoreMetaDataWarning = 1

        # optionally print message so that people know what file string to use to open in Ensight
        if self.__PrintEnsightFormatString:
            pm = servermanager.vtkProcessModule.GetProcessModule()
            pid = pm.GetGlobalController().GetLocalProcessId()
            if pid == 0:
                nump = pm.GetGlobalController().GetNumberOfProcesses()
                if nump == 1:
                    print("Ensight 'Set string' input is '", writer.FileName, ".*'", sep="")
                else:
                    print("Ensight 'Set string' input is '", writer.FileName, ".*."+str(nump)+ \
                          ".<"+str(nump)+":%0."+str(len(str(nump-1)))+"d>'", sep="")

    def RegisterWriter(self, writer, filename, freq, paddingamount=0, **params):
        """Registers a writer proxy. This method is generally used in
           CreatePipeline() to register writers. All writes created as such will
           write the output files appropriately in WriteData() is called."""
        writerParametersProxy = self.WriterParametersProxy(
            writer, filename, freq, paddingamount)

        writer.FileName = filename
        writer.add_attribute("parameters", writerParametersProxy)
        for p in params:
            v = params[p]
            if writer.GetProperty(p) is not None:
                wp = writer.GetProperty(p)
                wp.SetData(v)


        self.__WritersList.append(writer)

        helperName = writer.GetXMLName()
        if helperName == "ExodusIIWriter":
            self.ProcessExodusIIWriter(writer)

        return writer

    def WriterParametersProxy(self, writer, filename, freq, paddingamount):
        """Creates a client only proxy that will be synchronized with ParaView
        Live, allowing a user to set the filename and frequency.
        """
        controller = servermanager.ParaViewPipelineController()
        # assume that a client only proxy with the same name as a writer
        # is available in "insitu_writer_parameters"

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
            # it's possible that the writer can take in multiple input connections
            # so we need to go through all of them. the try/except block seems
            # to be the best way to figure out if there are multiple input connections
            try:
                length = len(writer.Input)
                for i in range(length):
                    proxy.GetProperty("Input").AddInputConnection(
                        writer.Input[i].SMProxy, 0)
            except:
                proxy.GetProperty("Input").SetInputConnection(
                    0, writer.Input.SMProxy, 0)
        proxy.GetProperty("FileName").SetElement(0, filename)
        proxy.GetProperty("WriteFrequency").SetElement(0, freq)

        proxy.GetProperty("PaddingAmount").SetElement(0, paddingamount)
        controller.PostInitializeProxy(proxy)
        controller.RegisterPipelineProxy(proxy)
        return proxy

    def RegisterCinemaTrack(self, name, proxy, smproperty, valrange):
        """
        Register a point of control (filter's property) that will be varied over in a cinema export.
        """
        if not isinstance(proxy, servermanager.Proxy):
            raise RuntimeError ("Invalid 'proxy' argument passed to RegisterCinemaTrack.")
        self.__CinemaTracksList.append({"name":name, "proxy":proxy, "smproperty":smproperty, "valrange":valrange})
        proxyDefinitions = self.__CinemaTracks[proxy] if (proxy in self.__CinemaTracks) else {}
        proxyDefinitions[smproperty] = valrange
        self.__CinemaTracks[proxy] = proxyDefinitions
        return proxy

    def AddArraysToCinemaTrack(self, proxy, propertyName, arrayNames):
        ''' Register user-defined target arrays by name. '''
        if not isinstance(proxy, servermanager.Proxy):
            raise RuntimeError ("Invalid 'proxy' argument passed to AddArraysToCinemaTrack.")

        proxyDefinitions = self.__CinemaTracks[proxy] if (proxy in self.__CinemaTracks) else {}
        proxyDefinitions[propertyName] = arrayNames
        self.__CinemaTracks[proxy] = proxyDefinitions
        return proxy

    def RegisterView(self, view, filename, freq, fittoscreen, magnification, width, height,
                     cinema=None, compression=None):
        """Register a view for image capture with extra meta-data such
        as magnification, size and frequency."""
        if not isinstance(view, servermanager.Proxy):
            raise RuntimeError ("Invalid 'view' argument passed to RegisterView.")
        view.add_attribute("cpFileName", filename)
        view.add_attribute("cpFrequency", freq)
        view.add_attribute("cpFitToScreen", fittoscreen)
        view.add_attribute("cpMagnification", magnification)
        view.add_attribute("cpCinemaOptions", cinema)
        view.add_attribute("cpCompression", compression)
        view.ViewSize = [ width, height ]
        self.__ViewsList.append(view)
        return view

    def CreateWriter(self, proxy_ctor, filename, freq):
        """**DEPRECATED!!! Use RegisterWriter instead**
        Creates a writer proxy. This method is generally used in
        reatePipeline() to create writers. All writes created as such will
        write the output files appropriately in WriteData() is called.
        """
        writer = proxy_ctor()
        return self.RegisterWriter(writer, filename, freq)

    def CreateView(self, proxy_ctor, filename, freq, fittoscreen, magnification, width, height):
        """**DEPRECATED!!! Use RegisterView instead**
        Create a CoProcessing view for image capture with extra meta-data
        such as magnification, size and frequency.
        """
        view = proxy_ctor()
        return self.RegisterView(view, filename, freq, fittoscreen, magnification, width, height, None)

    def Finalize(self):
        for writer in self.__WritersList:
            if hasattr(writer, 'Finalize'):
                writer.Finalize()
        for view in self.__ViewsList:
            if hasattr(view, 'Finalize'):
                view.Finalize()

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
                # rep is either not visible or not mapping scalars using a LUT.
                continue;

            input = rep.Input
            input.UpdatePipeline(time) #make sure range is up-to-date
            lut = rep.LookupTable

            colorArrayInfo = rep.GetArrayInformationForColorArray()
            if not colorArrayInfo:
                import sys
                datarange = [sys.float_info.max, -sys.float_info.max]
            else:
                if lut.VectorMode != 'Magnitude' or \
                   colorArrayInfo.GetNumberOfComponents() == 1:
                    datarange = colorArrayInfo.GetComponentRange(lut.VectorComponent)
                else:
                    # -1 corresponds to the magnitude.
                    datarange = colorArrayInfo.GetComponentRange(-1)

            from paraview.vtk import vtkDoubleArray
            import paraview.servermanager
            pm = paraview.servermanager.vtkProcessModule.GetProcessModule()
            globalController = pm.GetGlobalController()
            localarray = vtkDoubleArray()
            localarray.SetNumberOfTuples(2)
            localarray.SetValue(0, -datarange[0]) # negate so that MPI_MAX gets min instead of doing a MPI_MIN and MPI_MAX
            localarray.SetValue(1, datarange[1])
            globalarray = vtkDoubleArray()
            globalarray.SetNumberOfTuples(2)
            globalController.AllReduce(localarray, globalarray, 0)
            globaldatarange = [-globalarray.GetValue(0), globalarray.GetValue(1)]
            rgbpoints = lut.RGBPoints.GetData()
            numpts = len(rgbpoints)//4
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

    def UpdateCinema(self, view, datadescription, specLevel):
        """ called from catalyst at each timestep to add to the cinema database """
        if not view.IsA("vtkSMRenderViewProxy") == True:
            return

        try:
            import paraview.tpl.cinema_python.adaptors.explorers as explorers
            import paraview.tpl.cinema_python.adaptors.paraview.pv_explorers as pv_explorers
            import paraview.tpl.cinema_python.adaptors.paraview.pv_introspect as pv_introspect
        except ImportError as e:
            import paraview
            paraview.print_error("Cannot import cinema")
            paraview.print_error(e)
            return

        #figure out where to put this store
        import os.path
        vfname = view.cpFileName
        extension = os.path.splitext(vfname)[1]
        vfname = vfname[0:vfname.rfind("_")] #strip _num.ext
        fname = os.path.join(os.path.dirname(vfname),
                             "cinema",
                             os.path.basename(vfname),
                             "info.json")

        def float_limiter(x):
            #a shame, but needed to make sure python, javascript and (directory/file)name agree
            if isinstance(x, (float)):
                return '%.6e' % x #arbitrarily chose 6 significant digits
            else:
                return x

        #what time?
        time = datadescription.GetTime()
        view.ViewTime = time
        formatted_time = float_limiter(time)

        #ensure that cinema operates on the specified view
        simple.SetActiveView(view)

        # Include camera information in the user defined parameters.
        # pv_introspect uses __CinemaTracks to customize the exploration.
        co = view.cpCinemaOptions
        camType = co["camera"]
        if "phi" in co:
            self.__CinemaTracks["phi"] = co["phi"]
        if "theta" in co:
            self.__CinemaTracks["theta"] = co["theta"]
        if "roll" in co:
            self.__CinemaTracks["roll"] = co["roll"]

        tracking_def = {}
        if "tracking" in co:
            tracking_def = co['tracking']

        #figure out what we show now
        pxystate= pv_introspect.record_visibility()
        # a conservative global bounds for consistent z scaling
        minbds, maxbds  = pv_introspect.max_bounds()

        #make sure depth rasters are consistent
        view.MaxClipBounds = [minbds, maxbds, minbds, maxbds, minbds, maxbds]
        view.LockBounds = 1

        disableValues = False if 'noValues' not in co else co['noValues']

        if specLevel=="B":
            p = pv_introspect.inspect(skip_invisible=True)
        else:
            p = pv_introspect.inspect(skip_invisible=False)
        fs = pv_introspect.make_cinema_store(p, fname, view,
                                             forcetime = formatted_time,
                                             userDefined = self.__CinemaTracks,
                                             specLevel = specLevel,
                                             camType = camType,
                                             extension = extension,
                                             disableValues = disableValues)

        #all nodes participate, but only root can writes out the files
        pm = servermanager.vtkProcessModule.GetProcessModule()
        pid = pm.GetPartitionId()

        enableFloatVal = False if 'floatValues' not in co else co['floatValues']

        new_files = {}
        ret = pv_introspect.explore(fs, p, iSave = (pid == 0),
                              currentTime = {'time':formatted_time},
                              userDefined = self.__CinemaTracks,
                              specLevel = specLevel,
                              camType = camType,
                              tracking = tracking_def,
                              floatValues = enableFloatVal,
                              disableValues = disableValues)
        if pid == 0:
            fs.save()
        new_files[vfname] = ret;

        view.LockBounds = 0

        #restore what we showed
        pv_introspect.restore_visibility(pxystate)
        return os.path.basename(vfname), new_files

    def IsInModulo(self, datadescription, frequencies):
        """
        Return True if the given timestep in datadescription is in one of the provided frequencies
        or output is forced. This can be interpreted as follow::

        isFM = IsInModulo(timestep-timeStepToStartOutputAt, [2,3,7])

        is similar to::

        isFM = (timestep-timeStepToStartOutputAt % 2 == 0) or (timestep-timeStepToStartOutputAt % 3 == 0) or (timestep-timeStepToStartOutputAt % 7 == 0)

        The timeStepToStartOutputAt is the first timestep that will potentially be output.
        """
        timestep = datadescription.GetTimeStep()
        if timestep < self.__TimeStepToStartOutputAt and not self.__ForceOutputAtFirstCall:
            return False
        for frequency in frequencies:
            if frequency > 0 and self.NeedToOutput(datadescription, frequency):
                return True

        return False


    def NeedToOutput(self, datadescription, frequency):
        """
        Return True if we need to output based on the input timestep, frequency and forceOutput. Checks based
        __FirstTimeStepIndex, __FirstTimeStepIndex, __ForceOutputAtFirstCall and __TimeStepToStartOutputAt
        member variables.
        """
        if datadescription.GetForceOutput() == True:
            return True

        timestep = datadescription.GetTimeStep()
        if self.__FirstTimeStepIndex == None:
            self.__FirstTimeStepIndex = timestep

        if self.__ForceOutputAtFirstCall and self.__FirstTimeStepIndex == timestep:
            return True

        if self.__TimeStepToStartOutputAt <= timestep and (timestep-self.__TimeStepToStartOutputAt) % frequency == 0:
            return True

        return False


    def EnableCinemaDTable(self):
        """ Enable the normally disabled cinema D table export feature """
        self.__CinemaDHelper = exportnow.CinemaDHelper(True, self.__RootDirectory)


    def __AppendCViewToCinemaDTable(self, time, producer, filelist):
        """
        This is called every time catalyst writes any cinema image result
        to update the Cinema D index of outputs table.
        Note, we aggregate file operations later with __FinalizeCinemaDTable.
        """
        if self.__CinemaDHelper is None:
            return
        import vtk
        comm = vtk.vtkMultiProcessController.GetGlobalController()
        if comm.GetLocalProcessId() == 0:
            self.__CinemaDHelper.AppendCViewToCinemaDTable(time, producer, filelist)


    def __AppendToCinemaDTable(self, time, producer, filename):
        """
        This is called every time catalyst writes any data file or screenshot
        to update the Cinema D index of outputs table.
        Note, we aggregate file operations later with __FinalizeCinemaDTable.
        """
        if self.__CinemaDHelper is None:
            return
        import vtk
        comm = vtk.vtkMultiProcessController.GetGlobalController()
        if comm.GetLocalProcessId() == 0:
            self.__CinemaDHelper.AppendToCinemaDTable(time, producer, filename)

    def __FinalizeCinemaDTable(self):
        if self.__CinemaDHelper is None:
            return
        import vtk
        comm = vtk.vtkMultiProcessController.GetGlobalController()
        if comm.GetLocalProcessId() == 0:
            self.__CinemaDHelper.WriteNow()


    def SetRootDirectory(self, root_directory):
        """ Makes Catalyst put all output under this directory. """
        if root_directory is not '' and not root_directory.endswith("/"):
            root_directory = root_directory + "/"
        self.__RootDirectory = root_directory


    def __FixupWriters(self):
        """ Called once to ensure that all writers obey the root directory directive """
        if self.__RootDirectory is "":
            return
        for view in self.__ViewsList:
            view.cpFileName = self.__RootDirectory + view.cpFileName
        for writer in self.__WritersList:
            fileName = self.__RootDirectory + writer.parameters.GetProperty("FileName").GetElement(0)
            writer.parameters.GetProperty("FileName").SetElement(0, fileName)
            writer.parameters.FileName = fileName
