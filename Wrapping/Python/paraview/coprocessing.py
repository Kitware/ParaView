r"""
This module is designed for use in co-processing Python scripts. It provides a
class, Pipeline, which is designed to be used as the base-class for Python
pipeline. Additionally, this module has several other utility functions that are
approriate for co-processing.
"""

from paraview import simple, servermanager
from vtk.vtkPVVTKExtensionsCore import *
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
       implementation for the CreatePipeline() method.

	   Cinema Tracks
	   =============
	   CoProcessor maintains user-defined information for the Cinema generation in
       __CinemaTracks. This information includes track parameter values, data array
       names, etc. __CinemaTracks holds this information in the following structure:

        { proxy_reference : { 'ControlName' : [value_1, value_2, ..., value_n],
                              'arraySelection' : ['ArrayName_1', ..., 'ArrayName_n'] } }

		__CinemaTracks is populated when defining the co-processing pipline through
		paraview.cpstate. paraview.cpstate uses accessor instances to set values and
		array names through the RegisterCinemaTrack and AddArraysToCinemaTrack methods
        of this class.
    """

    def __init__(self):
        self.__PipelineCreated = False
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
        self.__InitialFrequencies = frequencies

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

        # if this is a time step to do live then all of the inputs
        # must be made available. note that we want the pipeline built
        # before we do the actual first live connection.
        if self.__EnableLiveVisualization and timestep % self.__LiveVisualizationFrequency == 0 \
           and self.__LiveVisualizationLink:
            if self.__LiveVisualizationLink.Initialize(servermanager.ActiveConnection.Session.GetSessionProxyManager()):
                num_inputs = datadescription.GetNumberOfInputDescriptions()
                for cc in range(num_inputs):
                    input_name = datadescription.GetInputDescriptionName(cc)
                    datadescription.GetInputDescription(cc).AllFieldsOn()
                    datadescription.GetInputDescription(cc).GenerateMeshOn()
                return

        # if we haven't processed the pipeline yet in DoCoProcessing() we
        # must use the initial frequencies to figure out if there's
        # work to do this time/timestep. If Live is enabled we mark
        # all inputs as needed (this is only done if the Live connection
        # hasn't been set up yet). If we don't have live enabled
        # we know that the output frequencies aren't changed and can
        # just use the initial frequencies.
        if self.__InitialFrequencies or not self.__EnableLiveVisualization:
            num_inputs = datadescription.GetNumberOfInputDescriptions()
            for cc in range(num_inputs):
                input_name = datadescription.GetInputDescriptionName(cc)

                freqs = self.__InitialFrequencies.get(input_name, [])
                if self.__EnableLiveVisualization or ( self and IsInModulo(timestep, freqs) ):
                        datadescription.GetInputDescription(cc).AllFieldsOn()
                        datadescription.GetInputDescription(cc).GenerateMeshOn()
        else:
            # the catalyst pipeline may have been changed by a live connection
            # so we need to regenerate the frequencies
            import cpstate
            frequencies = {}
            for writer in self.__WritersList:
                frequency = writer.parameters.GetProperty(
                    "WriteFrequency").GetElement(0)
                if (timestep % frequency) == 0 or \
                   datadescription.GetForceOutput() == True:
                    writerinputs = cpstate.locate_simulation_inputs(writer)
                    for writerinput in writerinputs:
                        datadescription.GetInputDescriptionByName(writerinput).AllFieldsOn()
                        datadescription.GetInputDescriptionByName(writerinput).GenerateMeshOn()

            for view in self.__ViewsList:
                if (view.cpFrequency and timestep % view.cpFrequency == 0) or \
                   datadescription.GetForceOutput() == True:
                    viewinputs = cpstate.locate_simulation_inputs(writer)
                    for viewinput in viewinputs:
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
        else:
            simtime = datadescription.GetTime()
            for name, producer in self.__ProducersMap.iteritems():
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
            if (timestep % frequency) == 0 or \
                    datadescription.GetForceOutput() == True:
                fileName = writer.parameters.GetProperty("FileName").GetElement(0)
                writer.FileName = fileName.replace("%t", str(timestep))
                writer.UpdatePipeline(datadescription.GetTime())

    def WriteImages(self, datadescription, rescale_lookuptable=False):
        """This method will update all views, if present and write output
            images, as needed."""
        timestep = datadescription.GetTimeStep()

        cinema_dirs = []
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
                cinemaOptions = view.cpCinemaOptions
                if cinemaOptions and 'camera' in cinemaOptions:
                    dirname = None
                    if 'composite' in cinemaOptions:
                        dirname = self.UpdateCinemaComposite(view, datadescription)
                    else:
                        dirname = self.UpdateCinema(view, datadescription)
                    if dirname:
                        cinema_dirs.append(dirname)
                else:
                    # for png quality = 0 means no compression. compression can be a potentially
                    # very costly serial operation on process 0
                    if fname.endswith('png'):
                        simple.SaveScreenshot(fname, view, magnification=view.cpMagnification, quality=0)
                    else:
                        simple.SaveScreenshot(fname, view, magnification=view.cpMagnification)


        if len(cinema_dirs) > 1:
            workspace = open('cinema/info.json', 'w')
            workspace.write('{\n')
            workspace.write('    "metadata": {\n')
            workspace.write('        "type": "workbench"\n')
            workspace.write('    },\n')
            workspace.write('    "runs": [\n')
            for i in range(0,len(cinema_dirs)):
                workspace.write('        {\n')
                workspace.write('        "title": "%s",\n' % cinema_dirs[i])
                workspace.write('        "description": "%s",\n' % cinema_dirs[i])
                workspace.write('        "path": "%s"\n' % cinema_dirs[i])
                if i+1 < len(cinema_dirs):
                    workspace.write('        },\n')
                else:
                    workspace.write('        }\n')
            workspace.write('    ]\n')
            workspace.write('}\n')
            workspace.close()


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


        timeStep = datadescription.GetTimeStep()
        if self.__EnableLiveVisualization and timeStep % self.__LiveVisualizationFrequency == 0:
            if not self.__LiveVisualizationLink.Initialize(servermanager.ActiveConnection.Session.GetSessionProxyManager()):
                return

        time = datadescription.GetTime()

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
            # it's possible that the writer can take in multiple input connections
            # so we need to go through all of them. the try/except block seems
            # to be the best way to figure out if there are multipel input connections
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
        controller.PostInitializeProxy(proxy)
        controller.RegisterPipelineProxy(proxy)
        return proxy

    def RegisterCinemaTrack(self, name, proxy, smproperty, valrange):
        """
        Register a point of control (filter's property) that will be varied over in a cinema export.
        """
        if not isinstance(proxy, servermanager.Proxy):
            raise RuntimeError, "Invalid 'proxy' argument passed to RegisterCinemaTrack."
        self.__CinemaTracksList.append({"name":name, "proxy":proxy, "smproperty":smproperty, "valrange":valrange})
        proxyDefinitions = self.__CinemaTracks[proxy] if (proxy in self.__CinemaTracks) else {}
        proxyDefinitions[smproperty] = valrange
        self.__CinemaTracks[proxy] = proxyDefinitions
        return proxy

    def AddArraysToCinemaTrack(self, proxy, propertyName, arrayNames):
        ''' Register user-defined target arrays by name. '''
        if not isinstance(proxy, servermanager.Proxy):
            raise RuntimeError, "Invalid 'proxy' argument passed to AddArraysToCinemaTrack."

        proxyDefinitions = self.__CinemaTracks[proxy] if (proxy in self.__CinemaTracks) else {}
        proxyDefinitions[propertyName] = arrayNames
        self.__CinemaTracks[proxy] = proxyDefinitions
        return proxy

    def RegisterView(self, view, filename, freq, fittoscreen, magnification, width, height, cinema=None):
        """Register a view for image capture with extra meta-data such
        as magnification, size and frequency."""
        if not isinstance(view, servermanager.Proxy):
            raise RuntimeError, "Invalid 'view' argument passed to RegisterView."
        view.add_attribute("cpFileName", filename)
        view.add_attribute("cpFrequency", freq)
        view.add_attribute("cpFileName", filename)
        view.add_attribute("cpFitToScreen", fittoscreen)
        view.add_attribute("cpMagnification", magnification)
        view.add_attribute("cpCinemaOptions", cinema)
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
                # rep is either not visibile or not mapping scalars using a LUT.
                continue;

            input = rep.Input
            input.UpdatePipeline(time) #make sure range is up-to-date
            lut = rep.LookupTable

            colorArrayInfo = rep.GetArrayInformationForColorArray()
            if not colorArrayInfo:
                continue

            if lut.VectorMode != 'Magnitude' or \
               colorArrayInfo.GetNumberOfComponents() == 1:
                datarange = colorArrayInfo.GetComponentRange(lut.VectorComponent)
            else:
                # -1 corresponds to the magnitude.
                datarange = colorArrayInfo.GetComponentRange(-1)

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

    def UpdateCinema(self, view, datadescription):
        """ called from catalyst at each timestep to add to the cinema "SPEC A" database """
        if not view.IsA("vtkSMRenderViewProxy") == True:
            return

        try:
            import paraview.cinemaIO.cinema_store as CS
            import paraview.cinemaIO.explorers as explorers
            import paraview.cinemaIO.pv_explorers as pv_explorers
        except ImportError as e:
            paraview.print_error("Cannot import cinema")
            paraview.print_error(e)
            return

        def get_nearest(eye, at, up, phis, thetas):
            """ returns phi and theta settings that most closely match current view """
            #todo: derive it instead of this brute force search
            best_phi = None
            best_theta = None
            best_dist = None
            best_up = None
            dist1 = math.sqrt(sum(math.pow(eye[x]-at[x],2) for x in [0,1,2]))
            for t,p in ((x,y) for x in thetas for y in phis):
                theta_rad = (float(t)) / 180.0 * math.pi
                phi_rad = float(p) / 180.0 * math.pi
                pos = [
                    float(at[0]) - math.cos(phi_rad)   * dist1 * math.cos(theta_rad),
                    float(at[1]) + math.sin(phi_rad)   * dist1 * math.cos(theta_rad),
                    float(at[2]) + math.sin(theta_rad) * dist1
                ]
                nup = [
                    + math.cos(phi_rad) * math.sin(theta_rad),
                    - math.sin(phi_rad) * math.sin(theta_rad),
                    + math.cos(theta_rad)
                ]
                dist = math.sqrt(sum(math.pow(eye[x]-pos[x],2) for x in [0,1,2]))
                updiff = math.sqrt(sum(math.pow(up[x]-nup[x],2) for x in [0,1,2]))
                if best_dist == None or (dist<best_dist and updiff<1.0):
                    best_phi = p
                    best_theta = t
                    best_dist = dist
                    best_up = updiff
            return best_phi, best_theta


        pm = servermanager.vtkProcessModule.GetProcessModule()
        pid = pm.GetPartitionId()

        #load or create the cinema store for this view
        import os.path
        vfname = view.cpFileName
        vfname = vfname[0:vfname.rfind("_")] #strip _num.ext

        fname = os.path.join(os.path.dirname(vfname),
                             "cinema",
                             os.path.basename(vfname),
                             "info.json")
        fs = CS.FileStore(fname)
        try:
            fs.load()
        except IOError:
            pass
        fs.add_metadata({'type':'parametric-image-stack'})

        def float_limiter(x):
            #a shame, but needed to make sure python, javascript and (directory/file)name agree
            if isinstance(x, (float)):
                #return '%6f' % x #arbitrarily chose 6 decimal places
                return '%.6e' % x #arbitrarily chose 6 significant digits
            else:
                return x

        #add record of current time to the store
        timestep = datadescription.GetTimeStep()
        time = datadescription.GetTime()
        view.ViewTime = time
        formatted_time = float_limiter(time)
        try:
            tprop = fs.get_parameter('time')
            tprop['values'].append(formatted_time)
        except KeyError:
            tprop = CS.make_parameter('time', [formatted_time])
            fs.add_parameter('time', tprop)

        parameters = []
        tracks = []

        #fixed track for time
        fnpattern = "{time}/"

        #make up track for each variable
        vals = []
        names = []
        for track in self.__CinemaTracksList:
            proxy = track['proxy']
            #rep = servermanager.GetRepresentation(proxy, view)
            #if not rep or rep.Visibility == 0:
            #    #skip if track if not visible in this view
            #    continue
            name = track['name']
            #make unique
            idx = 0
            while name in names:
                name = track['name'] + str(idx)
                idx = idx+1
            names.append(name)
            fnpattern = fnpattern + "{"+name+"}/"
            proxy = track['proxy']
            smproperty = track['smproperty']
            valrange = list(float_limiter(x for x in track['valrange']))
            fs.add_parameter(name, CS.make_parameter(name, valrange))
            parameters.append(name)
            tracks.append(pv_explorers.Templated(name, proxy, smproperty))
            #save off current value for later restoration
            vals.append([proxy, smproperty, list(proxy.GetPropertyValue(smproperty))])

        #make track for the camera rotation
        cinemaOptions = view.cpCinemaOptions
        if cinemaOptions and cinemaOptions.get('camera') == 'Spherical':
            fnpattern = fnpattern + "{phi}/{theta}/"
            if 'initial' in cinemaOptions:
                eye = cinemaOptions['initial']['eye']
                at = cinemaOptions['initial']['at']
                up = cinemaOptions['initial']['up']
                phis = list(float_limiter(x for x in cinemaOptions['phi']))
                thetas = list(float_limiter(x for x in cinemaOptions['theta']))
                best_phi, best_theta = get_nearest(eye, at, up, phis, thetas)
                fs.add_parameter("phi", CS.make_parameter('phi', phis, default=best_phi))
                fs.add_parameter("theta", CS.make_parameter('theta', thetas, default=best_theta))
            else:
                eye = view.CameraPosition
                at = view.CameraFocalPoint
                phis = list(float_limiter(x for x in cinemaOptions['phi']))
                thetas = list(float_limiter(x for x in cinemaOptions['theta']))
                fs.add_parameter("phi", CS.make_parameter('phi', phis))
                fs.add_parameter("theta", CS.make_parameter('theta', thetas))
            dist = math.sqrt(sum(math.pow(eye[x]-at[x],2) for x in [0,1,2]))
            #rectify for cinema exporter
            up = [math.fabs(x) for x in view.CameraViewUp]
            uppest = 0;
            if up[1]>up[uppest]: uppest = 1
            if up[2]>up[uppest]: uppest = 2
            cinup = [0,0,0]
            cinup[uppest]=1
            parameters.append("phi")
            parameters.append("theta")
            tracks.append(pv_explorers.Camera(at, cinup, dist, view))
            #save off current value for later restoration
            vals.append([view, 'CameraPosition', list(eye)])
            vals.append([view, 'CameraFocalPoint', list(at)])
            vals.append([view, 'CameraViewUp', list(up)])

        fnpattern = fnpattern[:-1] #strip trailing /
        imgext = view.cpFileName[view.cpFileName.rfind("."):]
        fnpattern = fnpattern + imgext
        fs.filename_pattern = fnpattern

        #at current time, run through parameters and dump files
        e = pv_explorers.ImageExplorer(fs, parameters, tracks, view=view, iSave=(pid==0))
        e.explore({'time':formatted_time})

        if pid == 0:
            fs.save()

        #restore values to what they were at beginning for next view
        for proxy, property, value in vals:
            proxy.SetPropertyWithName(property, value)

        return os.path.basename(vfname)

    def UpdateCinemaComposite(self, view, datadescription):
        """ called from catalyst at each timestep to add to the cinema "SPEC B" database """
        if not view.IsA("vtkSMRenderViewProxy") == True:
            return

        try:
            import paraview.cinemaIO.cinema_store as CS
            import paraview.cinemaIO.explorers as explorers
            import paraview.cinemaIO.pv_explorers as pv_explorers
            import paraview.cinemaIO.pv_introspect as pv_introspect
            import paraview.simple as simple
        except ImportError as e:
            paraview.print_error("Cannot import cinema")
            paraview.print_error(e)
            return


        #figure out where to put this store
        import os.path
        vfname = view.cpFileName
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
        timestep = datadescription.GetTimeStep()
        time = datadescription.GetTime()
        view.ViewTime = time
        formatted_time = float_limiter(time)

        # Include camera information in the user defined parameters.
		# pv_introspect uses __CinemaTracks to customize the exploration.
        co = view.cpCinemaOptions
        if "phi" in co:
            self.__CinemaTracks["phi"] = co["phi"]
        if "theta" in co:
            self.__CinemaTracks["theta"] = co["theta"]

        simple.Render(view)

        #figure out what we show now
        pxystate= pv_introspect.record_visibility()

        #make sure depth rasters are consistent
        view.LockBounds = 1

        p = pv_introspect.inspect()
        fs = pv_introspect.make_cinema_store(p, fname, forcetime = formatted_time,\
          userDefined = self.__CinemaTracks)

        #all nodes participate, but only root can writes out the files
        pm = servermanager.vtkProcessModule.GetProcessModule()
        pid = pm.GetPartitionId()

        pv_introspect.explore(fs, p, iSave = (pid == 0), currentTime = {'time':formatted_time},\
          userDefined = self.__CinemaTracks)
        if pid == 0:
            fs.save()

        view.LockBounds = 0

        #restore what we showed
        pv_introspect.restore_visibility(pxystate)
