r"""
This module is made to provide a set of tools and helper classes for data
exploration for Web deployment.
"""

import math, os, json, datetime, time
from paraview import simple, servermanager

#==============================================================================
# Run management
#==============================================================================

class AnalysisManager(object):
    """
    This class provide mechanism to keep track of a full data analysis and
    exploration so the generated analysis can be processed and viewed on
    the web.
    """
    def __init__(self, work_dir, title, description, **kwargs):
        """
        Create an Analysis manager that will dump its data
        within the provided work_dir.
        Then additional information can be added to the associated metadata
        in the form of keyword arguments.
        """
        self.can_not_write = (servermanager.vtkProcessModule.GetProcessModule().GetPartitionId() != 0)
        self.file_name_generators = {}
        self.work_dir = work_dir
        self.timers = {}
        self.analysis = {
            "path": work_dir,
            "title": title,
            "description": description,
            "analysis": []
        }
        for key, value in kwargs.iteritems():
            self.analysis[key] = value
        self.begin()


    def register_analysis(self, key, title, description, file_pattern, data_type):
        """
        Register a managed analysis.
        """
        path = os.path.join(self.work_dir, key)
        file_name_generator = FileNameGenerator(path, file_pattern)
        metadata = { "title": title, "description": description, "path": path, "id": key , "type": data_type }
        for _key in metadata:
            file_name_generator.add_meta_data(_key, metadata[_key])

        self.analysis['analysis'].append(metadata)
        self.file_name_generators[key] = file_name_generator

    def get_file_name_generator(self, key):
        """
        Retreive an analysis FileNameGenerator based on the key of a registered analysis.
        """
        return self.file_name_generators[key]

    def add_meta_data(self, key, value):
        """
        Update analysis metadata with additional information.
        """
        self.analysis[key] = value

    def begin_work(self, key):
        """
        Record the time that a given work is taking.
        If begin/end_work is called multiple time, a count
        will be kept but the time will just sum itself.

        If 2 begin_work with the same key, the second one will override the first one.
        """
        current_timer = None
        if key in self.timers:
            current_timer = self.timers[key]
        else:
            current_timer = { 'total_time': 0.0, 'work_count': 0 , 'last_begin': 0.0}
            self.timers[key] = current_timer

        # Update begin time
        current_timer['last_begin'] = time.time()


    def end_work(self, key):
        """
        Record the time that a given work is taking.
        If begin/end_work is called multiple time, a count
        will be kept but the time will just sum itself.

        If 2 end_work with the same key, the second one will
        be ignore as no begin_work will be done before.
        """
        delta = 0
        if key in self.timers:
            current_timer = self.timers[key]
            if current_timer['last_begin'] != 0.0:
                delta = time.time()
                delta -= current_timer['last_begin']
                current_timer['last_begin'] = 0.0
                current_timer['total_time'] += delta
                current_timer['work_count'] += 1
        return delta

    def get_time(self, key):
        """
        Return the number of second for a given task
        """
        return self.timers[key]['total_time']

    def begin(self):
        """
        Start the analysis time recording.
        """
        self.begin_work("full_analysis")

    def end(self):
        """
        End the analysis time recording and write associated metadata
        """
        self.end_work("full_analysis")
        self.write()

    def write(self):
        """
        Write metadata file in the work directory that describe all the
        analysis that have been performed.
        """
        if self.can_not_write:
            return

        # Update timer info first
        self.analysis['timers'] = {}
        for key in self.timers:
            self.analysis['timers'][key] = {'total_time': self.timers[key]['total_time'], 'work_count': self.timers[key]['work_count'] }

        # Update costs
        for key in self.file_name_generators:
            for analysis in self.analysis['analysis']:
                if analysis['id'] == key:
                    analysis['cost'] = self.file_name_generators[key].get_cost()

        # Write to disk
        metadata_file_path = os.path.join(self.work_dir, "info.json")
        with open(metadata_file_path, "w") as metadata_file:
            metadata_file.write(json.dumps(self.analysis))


#==============================================================================
# File Name management
#==============================================================================

class FileNameGenerator(object):
    """
    This class provide some methods to help build a unique file name
    which map to a given simulation state.
    """

    def __init__(self, working_dir, name_format):
        """
        working_dir: Directory where the generated files should be stored
        name_format: File name pattern using key word like follow:
           {theta}_{phi}.jpg
           {sliceColor}_{slicePosition}.jpg
           {contourBy}_{contourValue}_{theta}_{phi}.jpg
        """
        self.can_write = (servermanager.vtkProcessModule.GetProcessModule().GetPartitionId() == 0)
        self.working_dir = working_dir
        self.name_format = name_format
        self.arguments = {}
        self.active_arguments = {}
        self.metadata = {}
        self.cost = { "time": 0, "space": 0, "images": 0}
        if not os.path.exists(self.working_dir) and self.can_write:
            os.makedirs(self.working_dir)

    def add_meta_data(self, key, value):
        """
        Add aditional metadata information for the analysis
        """
        self.metadata[key] = value

    def set_active_arguments(self, **kwargs):
        """
        Overide all active arguments.
        """
        self.active_arguments = kwargs

    def update_active_arguments(self, store_value=True, **kwargs):
        """
        Update active arguments and extend arguments range.
        """
        for key, value in kwargs.iteritems():
            value_str = "{value}".format(value=value)
            self.active_arguments[key] = value_str
            if store_value:
                if key in self.arguments:
                    try:
                        self.arguments[key]["values"].index(value_str)
                    except ValueError:
                        self.arguments[key]["values"].append(value_str)
                else:
                    typeOfValues = "range"
                    if type(value) == type("String"):
                        typeOfValues = "list"
                    self.arguments[key] = {
                        "values" : [ value_str ],
                        "default": value_str,
                        "type": typeOfValues,
                        "label": key
                    }

    def update_label_arguments(self, **kwargs):
        """
        Update label arguments, but argument must exist first
        """
        for key, value in kwargs.iteritems():
            if key in self.arguments:
                self.arguments[key]["label"] = value

    def get_filename(self):
        """
        Return the name of the file based on the current active arguments
        """
        return self.name_format.format(**self.active_arguments)

    def get_fullpath(self):
        """
        Return the full path of the file based on the current active arguments
        """
        fullpath = os.path.join(self.working_dir, self.get_filename())
        if not os.path.exists(os.path.dirname(fullpath)) and self.can_write:
            os.makedirs(os.path.dirname(fullpath))
        return fullpath

    def get_directory(self):
        fullpath = os.path.join(self.working_dir, self.get_filename())
        if not os.path.exists(os.path.dirname(fullpath)) and self.can_write:
            os.makedirs(os.path.dirname(fullpath))
        return os.path.dirname(fullpath)

    def _is_image(self, name):
        return name.split('.')[-1].lower() in ['jpg', 'png', 'gif', 'jpeg', 'tiff']

    def add_file_cost(self):
        filePath = self.get_fullpath()
        if os.path.exists(filePath):
            if self._is_image(filePath):
                self.cost['images'] = self.cost['images'] + 1
            self.cost['space'] += os.stat(filePath).st_size

    def add_image_width(self, width):
        self.cost['image-width'] = width

    def add_time_cost(self, ts):
        self.cost['time'] += ts

    def get_cost(self):
        return self.cost

    def get_nth_directory(self, n):
        fullpath = os.path.join(self.working_dir, self.get_filename())
        dirname = os.path.dirname(fullpath)
        while n:
            n -= 1
            dirname = os.path.dirname(dirname)
        if not os.path.exists(dirname):
            os.makedirs(dirname)
        return dirname

    def write_metadata(self):
        """
        Write the info.json file in the working directory which contains
        the metadata of the current file usage with the arguments range.
        """
        if not self.can_write:
            return

        jsonObj = {
            "working_dir": self.working_dir,
            "name_pattern": self.name_format,
            "arguments": self.arguments,
            "metadata" : self.metadata,
            "cost": self.cost
        }
        metadata_file_path = os.path.join(self.working_dir, "info.json")
        with open(metadata_file_path, "w") as metadata_file:
            metadata_file.write(json.dumps(jsonObj))

#==============================================================================
# Camera management
#==============================================================================

class CameraHandler(object):

    def __init__(self, file_name_generator, view):
        self.file_name_generator = file_name_generator
        self.view = view
        self.active_index = 0
        self.number_of_index = 0
        self.callback = None

    def set_callback(self, callback):
        self.callback = callback

    def reset(self):
        self.active_index = -1

    def __iter__(self):
        return self

    def next(self):
        if self.active_index + 1 < self.number_of_index:
            self.active_index += 1

            return self.active_index * 100 / self.number_of_index

        raise StopIteration()

    def apply_position(self):
        if self.callback:
            self.callback(self.view.CameraPosition, self.view.CameraFocalPoint, self.view.CameraViewUp)

    def get_camera_keys(self):
        return []

    def set_view(self, view):
        self.view = view

    def get_camera_position(self):
        return self.view.CameraPosition

    def get_camera_view_up(self):
        return self.view.CameraViewUp

    def get_camera_focal_point(self):
        return self.view.CameraFocalPoint


# ==============================================================================

class ThreeSixtyCameraHandler(CameraHandler):
    def __init__(self, file_name_generator, view, phis, thetas, center, axis, distance):
        CameraHandler.__init__(self, file_name_generator, view)
        self.camera_positions = []
        self.camera_ups = []
        self.phi = []
        self.theta = []
        self.active_index = 0
        self.focal_point = center

        try:
            # Z => 0 | Y => 2 | X => 1
            self.offset = (axis.index(1) + 1 ) % 3
        except ValueError:
            raise Exception("Rotation axis not supported", axis)

        for theta in thetas:
            theta_rad = float(theta) / 180.0 * math.pi
            for phi in phis:
                phi_rad = float(phi) / 180.0 * math.pi

                pos = [
                    float(center[0]) - math.cos(phi_rad)   * distance * math.cos(theta_rad),
                    float(center[1]) + math.sin(phi_rad)   * distance * math.cos(theta_rad),
                    float(center[2]) + math.sin(theta_rad) * distance
                    ]
                up = [
                    + math.cos(phi_rad) * math.sin(theta_rad),
                    - math.sin(phi_rad) * math.sin(theta_rad),
                    + math.cos(theta_rad)
                    ]

                # Handle rotation around Z => 0 | Y => 2 | X => 1
                for i in range(self.offset):
                    pos.insert(0, pos.pop())
                    up.insert(0, up.pop())

                # Save informations
                self.camera_positions.append(pos)
                self.camera_ups.append(up)
                self.phi.append(phi)
                self.theta.append(theta + 90)

        self.number_of_index = len(self.phi)

    def get_camera_keys(self):
        return ['phi', 'theta']

    def apply_position(self):
        self.file_name_generator.update_active_arguments(phi=self.phi[self.active_index])
        self.file_name_generator.update_active_arguments(theta=self.theta[self.active_index])
        self.view.CameraPosition = self.camera_positions[self.active_index]
        self.view.CameraViewUp = self.camera_ups[self.active_index]
        self.view.CameraFocalPoint = self.focal_point

        super(ThreeSixtyCameraHandler, self).apply_position()

# ==============================================================================

class WobbleCameraHandler(CameraHandler):
    def __init__(self, file_name_generator, view, focal_point, view_up, camera_position, wobble_angle = 5.0):
        super(WobbleCameraHandler, self).__init__(file_name_generator, view)
        self.focal_point = focal_point
        self.view_up = view_up
        self.positions = []
        self.phi = []
        self.theta = []

        distance = math.sqrt(math.pow(camera_position[0] - focal_point[0], 2) + math.pow(camera_position[1] - focal_point[1], 2) + math.pow(camera_position[2] - focal_point[2], 2))
        delta = distance * math.sin(woble_angle / 180.0 * math.pi)

        delta_theta = [ view_up[0], view_up[1], view_up[2] ]
        theta_norm = math.sqrt(delta_theta[0]*delta_theta[0] + delta_theta[1]*delta_theta[1] + delta_theta[2]*delta_theta[2])
        delta_theta[0] *= delta / theta_norm
        delta_theta[1] *= delta / theta_norm
        delta_theta[2] *= delta / theta_norm

        a = camera_position[0] - focal_point[0]
        b = camera_position[1] - focal_point[1]
        c = camera_position[2] - focal_point[2]
        delta_phi = [b*view_up[2] - c*view_up[1], c*view_up[0] - a*view_up[2], a*view_up[1] - b*view_up[0]]
        phi_norm = math.sqrt(delta_phi[0]*delta_phi[0] + delta_phi[1]*delta_phi[1] + delta_phi[2]*delta_phi[2])
        delta_phi[0] *= delta / phi_norm
        delta_phi[1] *= delta / phi_norm
        delta_phi[2] *= delta / phi_norm

        for theta in [-1, 0, 1]:
            for phi in [-1, 0, 1]:
                pos = [ camera_position[0] + (float(theta) * delta_theta[0]) + (float(phi) * delta_phi[0]),
                        camera_position[1] + (float(theta) * delta_theta[1]) + (float(phi) * delta_phi[1]),
                        camera_position[2] + (float(theta) * delta_theta[2]) + (float(phi) * delta_phi[2])]

                # Save informations
                self.positions.append(pos)
                self.phi.append(phi*woble_angle)
                self.theta.append(theta*woble_angle)

        self.number_of_index = len(self.phi)

    def get_camera_keys(self):
        return ['phi', 'theta']

    def apply_position(self):
        self.file_name_generator.update_active_arguments(phi=self.phi[self.active_index])
        self.file_name_generator.update_active_arguments(theta=self.theta[self.active_index])
        self.view.CameraPosition = self.positions[self.active_index]
        self.view.CameraViewUp = self.view_up
        self.view.CameraFocalPoint = self.focal_point

        super(WobbleCameraHandler, self).apply_position()

# ==============================================================================

class FixCameraHandler(CameraHandler):
    def __init__(self, file_name_generator, view, focal_point, view_up, camera_position):
        super(WobbleCameraHandler, self).__init__(file_name_generator, view)
        self.focal_point = focal_point
        self.view_up = view_up
        self.camera_position = camera_position
        self.number_of_index = 1

    def apply_position(self):
        self.view.CameraPosition = self.camera_position
        self.view.CameraViewUp = self.view_up
        self.view.CameraFocalPoint = self.focal_point

        super(FixCameraHandler, self).apply_position()


#==============================================================================
# Data explorer
#==============================================================================

class SliceExplorer(object):
    """
    Class use to dump image stack of a data exploration. This data exploration
    is slicing the input data along an axis and save each slice as a new image
    keeping the view normal using parallel projection to disable the projection
    scaling.
    """

    def __init__(self, file_name_generator, view, data, colorByArray, steps=10, normal=[0.0,0.0,1.0], viewup=[0.0,1.0,0.0], bound_range=[0.0, 1.0], parallelScaleRatio = 1.5):
        """
        file_name_generator: the file name generator to use. Need to have ['sliceColor', 'slicePosition'] as keys.
        view: View proxy to render in
        data: Input proxy to process
        colorByArray: { "RTData": { "lut": ... , "type": 'POINT_DATA'} }
        steps: Number of slice along the given axis. Default 10
        normal: Slice plane normal. Default [0,0,1]
        viewup=[0.0,1.0,0.0]
        bound_range: Array of 2 percentage of the actual data bounds. Default full bounds [0.0, 1.0]
        scaleParallelProj
        """
        self.analysis = None
        self.view_proxy = view
        self.slice = simple.Slice( SliceType="Plane", Input=data, SliceOffsetValues=[0.0] )
        self.sliceRepresentation = simple.Show(self.slice)
        self.colorByArray = colorByArray
        self.parallelScaleRatio = parallelScaleRatio
        self.slice.SliceType.Normal = normal
        self.dataBounds = data.GetDataInformation().GetBounds()
        self.normal = normal
        self.viewup = viewup
        self.origin = [ self.dataBounds[0] + bound_range[0] * (self.dataBounds[1] - self.dataBounds[0]),
                        self.dataBounds[2] + bound_range[0] * (self.dataBounds[3] - self.dataBounds[2]),
                        self.dataBounds[4] + bound_range[0] * (self.dataBounds[5] - self.dataBounds[4]) ]
        self.number_of_steps = steps
        ratio = (bound_range[1] - bound_range[0]) / float(steps-1)
        self.origin_inc = [ normal[0] * ratio * (self.dataBounds[1] - self.dataBounds[0]),
                            normal[1] * ratio * (self.dataBounds[3] - self.dataBounds[2]),
                            normal[2] * ratio * (self.dataBounds[5] - self.dataBounds[4]) ]

        # Update file name pattern
        self.file_name_generator = file_name_generator
        self.file_name_generator.add_image_width(view.ViewSize[0])

    @staticmethod
    def list_arguments():
        return ['sliceColor', 'slicePosition']

    @staticmethod
    def get_data_type():
        return "parametric-image-stack"

    def add_attribute(self, name, value):
        setattr(self, name, value)

    def set_analysis(self, analysis):
        self.analysis = analysis

    def UpdatePipeline(self, time=0):
        """
        Probe dataset and dump images to the disk
        """
        if self.analysis:
            self.analysis.begin_work('SliceExplorer')

        simple.SetActiveView(self.view_proxy)
        self.file_name_generator.update_active_arguments(time=time)
        self.slice.SMProxy.InvokeEvent('UserEvent', 'HideWidget')
        self.view_proxy.CameraParallelProjection = 1
        self.view_proxy.CameraViewUp = self.viewup
        self.view_proxy.CameraFocalPoint = [ 0,0,0 ]
        self.view_proxy.CameraPosition = self.slice.SliceType.Normal
        self.slice.SliceType.Origin = [ (self.dataBounds[0] + self.dataBounds[1])/2,
                                        (self.dataBounds[2] + self.dataBounds[3])/2,
                                        (self.dataBounds[4] + self.dataBounds[5])/2 ]
        simple.Render(self.view_proxy)
        simple.ResetCamera(self.view_proxy)
        self.view_proxy.CameraParallelScale = self.view_proxy.CameraParallelScale / self.parallelScaleRatio

        for step in range(int(self.number_of_steps)):
            self.slice.SliceType.Origin = [ self.origin[0] + float(step) * self.origin_inc[0],
                                            self.origin[1] + float(step) * self.origin_inc[1],
                                            self.origin[2] + float(step) * self.origin_inc[2] ]

            # Loop over each color withour changing geometry
            for name in self.colorByArray:
                # Choose color by
                self.sliceRepresentation.ColorArrayName = (self.colorByArray[name]["type"], name)
                self.sliceRepresentation.LookupTable = self.colorByArray[name]["lut"]
                self.file_name_generator.update_active_arguments(sliceColor=name)

                # Update file name pattern
                self.file_name_generator.update_active_arguments(slicePosition=step)
                simple.Render()
                simple.WriteImage(self.file_name_generator.get_fullpath())
                self.file_name_generator.add_file_cost()

        # Generate metadata
        self.file_name_generator.update_label_arguments(sliceColor="Color by:")
        self.file_name_generator.write_metadata()
        self.view_proxy.CameraParallelProjection = 0

        if self.analysis:
            delta_t = self.analysis.end_work('SliceExplorer')
            self.file_name_generator.add_time_cost(delta_t)

#==============================================================================

class ContourExplorer(object):
    """
    Class used to explore data. This Explorer won't dump any images but can be used
    along with the ThreeSixtyImageStackExporter() like in the following example.

        w = simple.Wavelet()

        dataRange = [40.0, 270.0]
        arrayName = ('POINT_DATA', 'RTData')
        fileGenerator = FileNameGenerator('/tmp/iso', '{time}_{contourBy}_{contourValue}_{theta}_{phi}.jpg')

        cExplorer = ContourExplorer(fileGenerator, w, arrayName, dataRange, 25)
        proxy = cExplorer.getContour()
        rep = simple.Show(proxy)

        lut = simple.GetLookupTableForArray( "RTData", 1, RGBPoints=[43.34006881713867, 0.23, 0.299, 0.754, 160.01158714294434, 0.865, 0.865, 0.865, 276.68310546875, 0.706, 0.016, 0.15] )
        rep.LookupTable = lut
        rep.ColorArrayName = arrayName
        view = simple.Render()

        time = 0.0
        exp = ThreeSixtyImageStackExporter(fileGenerator, view, [0,0,0], 100, [0,0,1], [30, 45])
        for progress in cExplorer:
            exp.UpdatePipeline(time)
            print progress
    """

    def __init__(self, file_name_generator, data, contourBy, scalarRange=[0.0, 1.0], steps=10):
        """
        file_name_generator: the file name generator to use. Need to have ['contourBy', 'contourValue'] as keys.
        """
        self.analysis = None
        self.file_name_generator = file_name_generator
        self.contour = simple.Contour(Input=data, ContourBy=contourBy[1], ComputeScalars=1)
        if contourBy[0] == 'POINT_DATA':
            self.data_range = data.GetPointDataInformation().GetArray(contourBy[1]).GetRange()
        else:
            self.data_range = data.GetCellDataInformation().GetArray(contourBy[1]).GetRange()
        self.scalar_origin = scalarRange[0]
        self.scalar_incr = (scalarRange[1] - scalarRange[0]) / float(steps)
        self.number_of_steps = steps
        self.current_step = 0
        # Update file name pattern
        self.file_name_generator.update_active_arguments(contourBy=contourBy[1])
        self.file_name_generator.update_active_arguments(contourValue=self.scalar_origin)
        self.file_name_generator.update_label_arguments(contourValue=str(contourBy[1]))

    @staticmethod
    def list_arguments():
        return ['contourBy', 'contourValue']

    @staticmethod
    def get_data_type():
        return None

    def __iter__(self):
        return self

    def next(self):
        if self.current_step < self.number_of_steps:
            self.contour.Isosurfaces = [ self.scalar_origin + float(self.current_step)*self.scalar_incr ]
            self.current_step += 1

            # Update file name pattern
            self.file_name_generator.update_active_arguments(contourValue=self.contour.Isosurfaces[0])

            return self.current_step * 100 / self.number_of_steps

        raise StopIteration()

    def reset(self):
        self.current_step = 0

    def getContour(self):
        """
        Return the contour proxy.
        """
        return self.contour

    def add_attribute(self, name, value):
        setattr(self, name, value)

    def set_analysis(self, analysis):
        self.analysis = analysis

#==============================================================================
# Data explorer
#==============================================================================

class ImageResampler(object):
    """
    Class used to explore data. This Explorer will resample the original dataset
    and produce a stack of images that correspond to XY slice along with numerical
    data so 2D chart can be use to plot along a line on X, Y or Z.

        data_to_probe = simple.Wavelet()

        fileGenerator = FileNameGenerator('/tmp/probe-slice', '{time}/{field}/{slice}.{format}')
        array_colors = {
            "RTData": simple.GetLookupTableForArray( "RTData", 1, RGBPoints=[43.34006881713867, 0.23, 0.299, 0.754, 160.01158714294434, 0.865, 0.865, 0.865, 276.68310546875, 0.706, 0.016, 0.15] ),
        }

        exp = ImageResampler(fileGenerator, data_to_probe, [21,21,21], array_colors)
        exp.UpdatePipeline()
    """
    def __init__(self, file_name_generator, data_to_probe, sampling_dimesions, array_colors, nanColor = [0,0,0,0], custom_probing_bounds = None):
        self.analysis = None
        self.file_name_generator = file_name_generator
        self.data_to_probe = data_to_probe
        self.array_colors = array_colors
        self.custom_probing_bounds = custom_probing_bounds
        self.number_of_slices = sampling_dimesions[2]
        self.resampler = simple.ImageResampling(Input=data_to_probe, SamplingDimension=sampling_dimesions)
        if custom_probing_bounds:
            self.resampler.UseInputBounds = 0
            self.resampler.CustomSamplingBounds = custom_probing_bounds
        field = array_colors.keys()[0]
        self.color = simple.ColorByArray(Input=self.resampler, LookupTable=array_colors[field], RGBANaNColor=nanColor, ColorBy=field )

        self.file_name_generator.add_image_width(sampling_dimesions[0])

    @staticmethod
    def list_arguments():
        return ['field', 'slice', 'format']

    @staticmethod
    def get_data_type():
        return "image-data-stack"

    def add_attribute(self, name, value):
        setattr(self, name, value)

    def set_analysis(self, analysis):
        self.analysis = analysis

    def UpdatePipeline(self, time=0):
        """
        Probe dataset and dump images + json files to the disk
        """
        if self.analysis:
            self.analysis.begin_work('ImageResampler')
        self.file_name_generator.update_active_arguments(time=time)
        self.resampler.UpdatePipeline(time)

        # Write resampled data as JSON files
        self.file_name_generator.update_active_arguments(format='json')
        writer = simple.JSONImageWriter(Input=self.resampler)
        for field in self.array_colors:
            self.file_name_generator.update_active_arguments(field=field)
            for slice in range(self.number_of_slices):
                self.file_name_generator.update_active_arguments(slice=slice)
                writer.FileName = self.file_name_generator.get_fullpath()
                writer.Slice = slice
                writer.ArrayName = field
                writer.UpdatePipeline(time)
                self.file_name_generator.add_file_cost()

        # Write image stack
        self.file_name_generator.update_active_arguments(format='jpg')
        for field in self.array_colors:
            self.file_name_generator.update_active_arguments(field=field)
            self.file_name_generator.update_active_arguments(False, slice='%d')
            self.color.LookupTable = self.array_colors[field]
            self.color.ColorBy = field
            self.color.UpdatePipeline(time)
            writer = simple.JPEGWriter(Input=self.color, FileName=self.file_name_generator.get_fullpath())
            writer.UpdatePipeline(time)
            for slice in range(self.number_of_slices):
                self.file_name_generator.update_active_arguments(slice=slice)
                self.file_name_generator.add_file_cost()

        # Generate metadata
        self.file_name_generator.write_metadata()

        if self.analysis:
            delta_t = self.analysis.end_work('ImageResampler')
            self.file_name_generator.add_time_cost(delta_t)

#==============================================================================
# Chart generator
#==============================================================================

class LineProber(object):
    def __init__(self, file_name_generator, data_to_probe, points_series, number_of_points):
        """
        file_name_generator: the file name generator to use. Need to have ['phi', 'theta'] as keys.
        data_to_probe: Input data proxy to probe.
        points_series: Data structure providing the location to probe.
            [ { name: "X_axis", start_point: [0.0, 0.0, 0.0], end_point: [1.0, 0.0, 0.0] },
              { name: "Y_axis", start_point: [0.0, 0.0, 0.0], end_point: [1.0, 0.0, 0.0] },
              { name: "Random", start_point: [0.0, 0.0, 0.0], end_point: [1.0, 0.0, 0.0] } ]
        fields: Array containing the name of the scalar value to extract.
            [ 'temperature', 'salinity' ]
        number_of_points: Number of points within the line
        """
        self.analysis = None
        self.file_name_generator = file_name_generator
        self.data_proxy = data_to_probe
        self.series = points_series
        self.probe = simple.PlotOverLine( Source="High Resolution Line Source", Input=data_to_probe )
        self.probe.SMProxy.InvokeEvent('UserEvent', 'HideWidget')
        self.probe.Source.Resolution = number_of_points

    def add_attribute(self, name, value):
        setattr(self, name, value)

    def set_analysis(self, analysis):
        self.analysis = analysis

    @staticmethod
    def list_arguments():
        return ['time', 'serie', 'field']

    @staticmethod
    def get_data_type():
        return "line-prober-csv-stack"

    def UpdatePipeline(self, time=0):
        """
        Write a file (x_{time}_{serie}.csv => x_134.0_X_Axis.csv) with the following format:

        idx,fieldA,fieldB,fieldC,fieldD,point0, point1, point2
        0,3.56,234,2565,5678,678,0,0,0
        1,3.57,234,2565,5678,678,1,2,4
        ...
        100,5.76,234,2565,5678,678,5,7,8

        """
        if self.analysis:
            self.analysis.begin_work('LineProber')

        self.file_name_generator.update_active_arguments(time=time)

        simple.SetActiveView(self.view_proxy)
        # Explore the data
        for serie in self.series:
            self.probe.Source.Point1 = serie['start_point']
            self.probe.Source.Point2 = serie['end_point']
            self.file_name_generator.update_active_arguments(serie=serie['name'])

            # Write CSV file
            writer = simple.DataSetCSVWriter(FileName=self.file_name_generator.get_fullpath(), Input=self.probe)
            writer.UpdatePipeline(time)
            self.file_name_generator.add_file_cost()

        # Get fields info
        pdInfo = self.probe.GetPointDataInformation()
        for array in pdInfo:
            self.file_name_generator.update_active_arguments(field=array.Name)

        # Generate metadata
        self.file_name_generator.write_metadata()

        if self.analysis:
            delta_t = self.analysis.end_work('LineProber')
            self.file_name_generator.add_time_cost(delta_t)

#==============================================================================

class DataProber(object):
    def __init__(self, file_name_generator, data_to_probe, points_series, fields):
        """
        file_name_generator: the file name generator to use. Need to have ['phi', 'theta'] as keys.
        data_to_probe: Input data proxy to probe.
        points_series: Data structure providing the location to probe.
            [ { name: "X_axis", probes: [ [0.0, 0.0, 0.0], [1.0, 0.0, 0.0], ... [100.0, 0.0, 0.0] ] },
              { name: "Y_axis", probes: [ [0.0, 0.0, 0.0], [0.0, 1.0, 0.0], ... [0.0, 100.0, 0.0] ] },
              { name: "Random", probes: [ [0.0, 0.0, 0.0], [0.0, 2.0, 1.0], ... [0.0, 50.0, 100.0] ] } ]
        fields: Array containing the name of the scalar value to extract.
            [ 'temperature', 'salinity' ]
        """
        self.analysis = None
        self.file_name_generator = file_name_generator
        self.data_proxy = data_to_probe
        self.series = points_series
        self.fields = fields
        self.probe = simple.ProbeLocation( Input=data_to_probe, ProbeType="Fixed Radius Point Source" )
        self.probe.SMProxy.InvokeEvent('UserEvent', 'HideWidget')
        self.location = self.probe.ProbeType
        self.location.NumberOfPoints = 1
        self.location.Radius = 0.0

    def add_attribute(self, name, value):
        setattr(self, name, value)

    def set_analysis(self, analysis):
        self.analysis = analysis

    @staticmethod
    def list_arguments():
        return ['time', 'field', 'serie']

    @staticmethod
    def get_data_type():
        return "point-series-prober-csv-stack"

    def UpdatePipeline(self, time=0):
        """
        Write a file (x_{time}_{field}_{serie}.csv => x_134.0_temperature_X_Axis.csv) with the following format:

        idx,X_axis
        0,3.56
        1,3.57
        ...
        100,5.76

        ++++

        idx,Y_axis
        0,5.6
        1,7.57
        ...
        100,10.76

        """
        if self.analysis:
            self.analysis.begin_work('DataProber')

        self.file_name_generator.update_active_arguments(time=time)

        # Explore the data
        saved_data = {}
        for serie in self.series:
            serie_name = serie['name']
            serie_points = serie['probes']

            # Create empty container
            saved_data[serie_name] = {}
            for field in self.fields:
                saved_data[serie_name][field] = []

            # Fill container with values
            for point in serie_points:
                if point:
                    self.location.Center = point
                    dataInfo = self.probe.GetPointDataInformation()
                    for field in self.fields:
                        array = dataInfo.GetArray(field)
                        saved_data[serie_name][field].append(array.GetRange(-1)[0])
                else:
                    # Just a NaN value
                    for field in self.fields:
                        saved_data[serie_name][field].append(float('NaN'))


        # Write data to disk
        for field in self.fields:
            self.file_name_generator.update_active_arguments(field=field)
            for serie in self.series:
                self.file_name_generator.update_active_arguments(serie=serie['name'])

                with open(self.file_name_generator.get_fullpath(), "w") as data_file:
                    data_file.write("idx,%s\n" % serie['name'])
                    idx = 0
                    for value in saved_data[serie['name']][field]:
                        data_file.write("%f,%f\n" % (idx, value))
                        idx += 1
                    self.file_name_generator.add_file_cost()

        # Generate metadata
        self.file_name_generator.write_metadata()

        if self.analysis:
            delta_t = self.analysis.end_work('DataProber')
            self.file_name_generator.add_time_cost(delta_t)

#==============================================================================

class TimeSerieDataProber(object):
    def __init__(self, file_name_generator, data_to_probe, point_series, fields, time_to_write):
        """
        file_name_generator: the file name generator to use. Need to have ['phi', 'theta'] as keys.
        data_to_probe: Input data proxy to probe.
        point_series: Data structure providing the location to probe.
            [ { name: "Origin", probe: [ 0.0, 0.0, 0.0 ] },
              { name: "Center", probe: [ 100.0, 100.0, 100.0 ]},
              { name: "Random", probe: [ 3.0, 2.0, 1.0 ] } ]
        fields: Array containing the name of the scalar value to extract.
            [ 'temperature', 'salinity' ]
        time_to_write: Give the time when the TimeSerieDataProber should dump the data to disk
                       at a given UpdatePipeline(time=2345) call.
        """
        self.analysis = None
        self.file_name_generator = file_name_generator
        self.data_proxy = data_to_probe
        self.fields = fields
        self.probe = simple.ProbeLocation( Input=data_to_probe, ProbeType="Fixed Radius Point Source" )
        self.probe.SMProxy.InvokeEvent('UserEvent', 'HideWidget')
        self.location = self.probe.ProbeType
        self.location.NumberOfPoints = 1
        self.location.Radius = 0.0
        self.data_arrays = {}
        self.time_to_write = time_to_write
        self.serie_names = [ 'time' ]
        self.points = []
        for serie in point_series:
            self.serie_names.append(serie['name'])
            self.points.append(serie['probe'])
        for field in self.fields:
            self.data_arrays[field] = [ self.serie_names ]


    def add_attribute(self, name, value):
        setattr(self, name, value)

    def set_analysis(self, analysis):
        self.analysis = analysis

    @staticmethod
    def list_arguments():
        return ['field']

    @staticmethod
    def get_data_type():
        return "time-csv-stack"

    def WriteToDisk(self):
        # Generate metadata
        self.file_name_generator.write_metadata()

        # Generate real data
        for field in self.fields:
            self.file_name_generator.update_active_arguments(field=field)
            arrays = self.data_arrays[field]
            with open(self.file_name_generator.get_fullpath(), "w") as data_file:
                for line in arrays:
                    data_file.write(",".join(line))
                    data_file.write("\n")
                self.file_name_generator.add_file_cost()

    def UpdatePipeline(self, time=0):
        """
        Write a file (x_{field}.csv => x_temperature.csv + x_salinity.csv) with the following format:

        time, Origin, Center, Random
        0.0, 3.56, 6.67, 2.4765
        0.5, 3.57, 6.65, 2.4755
        ...
        100.5, 5.76, 10.45, 5.567

        """
        if self.analysis:
            self.analysis.begin_work('TimeSerieDataProber')

        for field in self.fields:
            self.data_arrays[field].append([ "%f" % time ])

        for point in self.points:
            self.location.Center = point
            dataInfo = self.probe.GetPointDataInformation()
            for field in self.fields:
                array = dataInfo.GetArray(field)
                self.data_arrays[field][-1].append("%f"% array.GetRange(-1)[0])

        # Write to disk if need be
        if time >= self.time_to_write:
            self.WriteToDisk()

        if self.analysis:
            delta_t = self.analysis.end_work('TimeSerieDataProber')
            self.file_name_generator.add_time_cost(delta_t)

#==============================================================================
# Image composite
#==============================================================================

class CompositeImageExporter(object):
    """
    Class use to dump an image stack for a given view position so it can be
    recomposed later on in the web.
    We assume the RGBZView plugin is loaded.
    """
    def __init__(self, file_name_generator, data_list, colorBy_list, luts, camera_handler, view_size, data_list_pipeline, axisVisibility=1, orientationVisibility=1, format='jpg'):
        '''
        data_list = [ds1, ds2, ds3]
        colorBy_list = [ [('POINT_DATA', 'temperature'), ('POINT_DATA', 'pressure'), ('POINT_DATA', 'salinity')],
                         [('CELL_DATA', 'temperature'), ('CELL_DATA', 'salinity')],
                         [('SOLID_COLOR', [0.1,0.1,0.1])] ]
        luts = { 'temperature': vtkLookupTable(...),  'salinity': vtkLookupTable(...),  'pressure': vtkLookupTable(...), }
        data_list_pipeline = [ { 'name': 'Earth core'}, {'name': 'Slice'}, {'name': 'T=0', 'parent': 'Contour by temperature'}, ...]
        format = ['jpg', 'tiff', 'png']
        '''
        self.file_name_generator = file_name_generator
        self.datasets = data_list
        self.camera_handler = camera_handler
        self.analysis = None
        self.color_by = colorBy_list
        self.luts = luts

        # Create view and assembly manager
        self.view = simple.CreateView("RGBZView")
        self.view.ImageFormatExtension = format
        self.view.ViewSize = view_size
        self.view.CenterAxesVisibility = axisVisibility
        self.view.OrientationAxesVisibility = orientationVisibility
        self.view.UpdatePropertyInformation()
        self.codes = self.view.GetProperty('RepresentationCodes').GetData()
        self.camera_handler.set_view(self.view)
        self.representations = []

        index = 0
        for data in self.datasets:
            rep = simple.Show(data, self.view)
            self.representations.append(rep)
            self.file_name_generator.update_active_arguments(layer=self.codes[index])
            index += 1

        # Create pipeline metadata
        pipeline = []
        parentTree = {}
        index = 1
        for node in data_list_pipeline:
            entry = { 'name': node['name'], 'ids': [self.codes[index]], 'type': 'layer' }

            if 'parent' in node:
                if node['parent'] in parentTree:
                    # add node as child
                    parentTree[node['parent']]['children'].append(entry)
                    parentTree[node['parent']]['ids'].append(entry['ids'][0])
                else:
                    # register parent and add it to pipeline
                    directory = { 'name': node['parent'], 'type': 'directory', 'ids': [ entry['ids'][0] ], 'children': [ entry ]}
                    pipeline.append(directory)
                    parentTree[node['parent']] = directory
            else:
                # Add it to pipeline
                pipeline.append(entry)

            index += 1

        self.file_name_generator.add_meta_data('pipeline', pipeline)
        self.file_name_generator.add_meta_data('dimensions', view_size)
        self.file_name_generator.add_image_width(view_size[0])

        # Add the name of both generated files
        self.file_name_generator.update_active_arguments(filename='rgb.%s' % format)
        self.file_name_generator.update_active_arguments(filename='composite.json')

    @staticmethod
    def list_arguments():
        """

        '/path/{camera_handlers}/{filename}' + '/path/info.json'
        Where filename = ['composite.json', 'rgb.jpg']
        """
        return ['filename']

    @staticmethod
    def get_data_type():
        return "composite-image-stack"

    def add_attribute(self, name, value):
        setattr(self, name, value)

    def set_analysis(self, analysis):
        self.analysis = analysis

    def UpdatePipeline(self, time=0):
        """
        Change camera position and dump images to the disk
        """
        if self.analysis:
            self.analysis.begin_work('CompositeImageExporter')
        self.file_name_generator.update_active_arguments(time=time)

        # Fix camera bounds
        simple.Render(self.view)
        self.view.ResetClippingBounds()
        self.view.FreezeGeometryBounds()
        self.view.UpdatePropertyInformation()

        # Compute the number of images by stack
        fieldset = set()
        rangeset = {}

        nbImages = 1
        composite_size = len(self.representations)
        metadata_layers = ""
        for compositeIdx in range(composite_size):
            metadata_layers += self.codes[compositeIdx + 1]
            for field in self.color_by[compositeIdx]:
                fieldset.add(str(field[1]))
                nbImages += 1

        # Generate fields metadata
        index = 1
        metadata_fields = {}
        reverse_field_map = {}
        for field in fieldset:
            metadata_fields[self.codes[index]] = field
            reverse_field_map[field] = self.codes[index]
            index += 1

        # Generate layer_fields metadata
        metadata_layer_fields = {}
        for compositeIdx in range(composite_size):
            key = self.codes[compositeIdx + 1]
            array = []
            for field in self.color_by[compositeIdx]:
                array.append(reverse_field_map[str(field[1])])
            metadata_layer_fields[key] = array

        # Generate the heavy data
        metadata_offset = {}
        self.camera_handler.reset()
        for progress in self.camera_handler:
            self.camera_handler.apply_position()
            self.view.CompositeDirectory = self.file_name_generator.get_directory()

            # Extract images for each fields
            self.view.ResetActiveImageStack()
            self.view.RGBStackSize = nbImages
            offset_value = 1
            for compositeIdx in range(composite_size):
                rep = self.representations[compositeIdx]
                offset_composite_name = self.codes[compositeIdx + 1]
                index = 0
                for field in self.color_by[compositeIdx]:
                    index += 1
                    offset_field_name = reverse_field_map[str(field[1])]
                    if field[0] == 'SOLID_COLOR':
                        self.file_name_generator.update_active_arguments(field= "solid_" + str(index))
                        rep.AmbientColor = field[1]
                        rep.DiffuseColor = field[1]
                        rep.SpecularColor = field[1]
                    elif field[0] == 'VALUE':
                        #remember range for later storage in info file
                        rangeset[field[1]] = self.luts[field[1]][3]
                    else:
                        self.file_name_generator.update_active_arguments(field=field[1])
                        data_bounds = self.datasets[compositeIdx].GetDataInformation().GetBounds()
                        if data_bounds[0] > data_bounds[1]:
                            rep.ColorArrayName = ''
                        else:
                            rep.LookupTable = self.luts[field[1]]
                            rep.ColorArrayName = field

                    self.view.ActiveRepresentation = rep
                    self.view.CompositeDirectory = self.file_name_generator.get_directory()
                    if field[0] == 'VALUE':
                        rep.DiffuseColor = [1,1,1]
                        specifics = self.luts[field[1]]
                        if specifics[0] == "point":
                            self.view.SetDrawCells = 0
                        else:
                            self.view.SetDrawCells = 1
                        self.view.SetArrayNameToDraw = specifics[1]
                        self.view.SetArrayComponentToDraw = specifics[2]
                        self.view.SetScalarRange = specifics[3]
                        self.view.StartCaptureValues()
                        self.view.CaptureActiveRepresentation()
                        self.view.StopCaptureValues()
                    else:
                        self.view.CaptureActiveRepresentation()

                    metadata_offset[offset_composite_name + offset_field_name] = offset_value
                    offset_value += 1

            # Extract RGB + Z-buffer
            self.view.WriteImage()
            self.view.ComputeZOrdering()
            self.view.WriteComposite()

            # Add missing files in size computation
            self.file_name_generator.update_active_arguments(filename='composite.json')
            self.file_name_generator.add_file_cost()
            self.file_name_generator.update_active_arguments(filename='rgb.jpg')
            self.file_name_generator.add_file_cost()

        # Generate metadata
        self.file_name_generator.add_meta_data('fields', metadata_fields)
        self.file_name_generator.add_meta_data('layers', metadata_layers)
        self.file_name_generator.add_meta_data('layer_fields', metadata_layer_fields)
        self.file_name_generator.add_meta_data('offset', metadata_offset)
        self.file_name_generator.add_meta_data('ranges', rangeset)
        self.file_name_generator.write_metadata()

        if self.analysis:
            delta_t = self.analysis.end_work('CompositeImageExporter')
            self.file_name_generator.add_time_cost(delta_t)

#==============================================================================
# Image exporter
#==============================================================================

class ThreeSixtyImageStackExporter(object):
    """
    Class use to dump image stack of geometry exploration.
    This exporter will use the provided view to create a 360 view of the visible data.
    """

    def __init__(self, file_name_generator, view_proxy, focal_point=[0.0,0.0,0.0], distance=100.0, rotation_axis=[0,0,1], angular_steps=[10,15]):
        """
        file_name_generator: the file name generator to use. Need to have ['phi', 'theta'] as keys.
        view_proxy: View that will be used for the image captures.
        focal_point=[0.0,0.0,0.0]: Center of rotation and camera focal point.
        distance=100.0: Distance from where the camera should orbit.
        rotation_axis=[0,0,1]: Main axis around which the camera should orbit.
        angular_steps=[10,15]: Phi and Theta angular step in degre. Phi is the angle around
                               the main axis of rotation while Theta is the angle the camera
                               is looking at this axis.
        """
        self.analysis = None
        self.file_name_generator = file_name_generator
        self.angular_steps       = angular_steps
        self.focal_point         = focal_point
        self.distance            = float(distance)
        self.view_proxy          = view_proxy
        self.phi_rotation_axis   = rotation_axis
        try:
            # Z => 0 | Y => 2 | X => 1
            self.offset = (self.phi_rotation_axis.index(1) + 1 ) % 3
        except ValueError:
            raise Exception("Rotation axis not supported", self.phi_rotation_axis)

        self.file_name_generator.add_image_width(view_proxy.ViewSize[0])

    @staticmethod
    def list_arguments():
        return ['phi', 'theta']

    @staticmethod
    def get_data_type():
        return "catalyst-viewer"

    def add_attribute(self, name, value):
        setattr(self, name, value)

    def set_analysis(self, analysis):
        self.analysis = analysis

    def UpdatePipeline(self, time=0):
        """
        Change camera position and dump images to the disk
        """
        if self.analysis:
            self.analysis.begin_work('ThreeSixtyImageStackExporter')

        simple.SetActiveView(self.view_proxy)
        self.file_name_generator.update_active_arguments(time=time)
        self.view_proxy.CameraFocalPoint = self.focal_point
        self.view_proxy.CameraViewUp     = self.phi_rotation_axis
        theta_offset = 90 % self.angular_steps[1]
        if theta_offset == 0:
            theta_offset += self.angular_steps[1]
        for theta in range(-90 + theta_offset, 90 - theta_offset + 1, self.angular_steps[1]):
            theta_rad = float(theta) / 180.0 * math.pi
            for phi in range(0, 360, self.angular_steps[0]):
                phi_rad = float(phi) / 180.0 * math.pi

                pos = [
                    float(self.focal_point[0]) - math.cos(phi_rad)   * self.distance * math.cos(theta_rad),
                    float(self.focal_point[1]) + math.sin(phi_rad)   * self.distance * math.cos(theta_rad),
                    float(self.focal_point[2]) + math.sin(theta_rad) * self.distance
                    ]
                up = [
                    + math.cos(phi_rad) * math.sin(theta_rad),
                    - math.sin(phi_rad) * math.sin(theta_rad),
                    + math.cos(theta_rad)
                    ]

                # Handle rotation around Z => 0 | Y => 2 | X => 1
                for i in range(self.offset):
                    pos.insert(0, pos.pop())
                    up.insert(0, up.pop())

                # Apply new camera position
                self.view_proxy.CameraPosition = pos
                self.view_proxy.CameraViewUp = up
                simple.Render()

                # Update file name pattern
                self.file_name_generator.update_active_arguments(phi=phi, theta=(90+theta))
                simple.WriteImage(self.file_name_generator.get_fullpath())
                self.file_name_generator.add_file_cost()

        # Generate metadata
        self.file_name_generator.write_metadata()

        if self.analysis:
            delta_t = self.analysis.end_work('ThreeSixtyImageStackExporter')
            self.file_name_generator.add_time_cost(delta_t)

# -----------------------------------------------------------------------------

def test(basePath):
    w = simple.Wavelet()

    dataRange = [60.0, 250.0]
    arrayName = ('POINT_DATA', 'RTData')
    fileGenerator = FileNameGenerator(os.path.join(basePath, 'iso'), '{contourBy}/{contourValue}/{theta}_{phi}.jpg')

    cExplorer = ContourExplorer(fileGenerator, w, arrayName, dataRange, 10)
    proxy = cExplorer.getContour()
    view = simple.CreateRenderView()
    rep = simple.Show(proxy, view)

    lut = simple.GetLookupTableForArray( "RTData", 1, RGBPoints=[43.34006881713867, 0.23, 0.299, 0.754, 160.01158714294434, 0.865, 0.865, 0.865, 276.68310546875, 0.706, 0.016, 0.15] )
    rep.LookupTable = lut
    rep.ColorArrayName = arrayName
    simple.Render(view)

    exp = ThreeSixtyImageStackExporter(fileGenerator, view, [0,0,0], 100, [0,0,1], [30, 45])
    for progress in cExplorer:
        exp.UpdatePipeline()

# -----------------------------------------------------------------------------

def test2(basePath):
    min = 63.96153259277344
    max = 250.22056579589844
    w = simple.Wavelet()
    c = simple.Contour(Input=w, ComputeScalars=1, PointMergeMethod="Uniform Binning", ContourBy = ['POINTS', 'RTData'], Isosurfaces=[ 80.0, 100.0, 120.0, 140.0, 160.0, 180.0, 200.0, 220.0, 240.0 ])
    view = simple.CreateRenderView()
    simple.Show(w,view)
    r = simple.Show(c,view)
    simple.Render(view)

    lut = simple.GetLookupTableForArray(
        "RTData", 1,
        RGBPoints=[min, 0.23, 0.299, 0.754, (min+max)*0.5, 0.865, 0.865, 0.865, max, 0.706, 0.016, 0.15],
        ColorSpace='Diverging',
        ScalarRangeInitialized=1.0 )

    exp = ThreeSixtyImageStackExporter(FileNameGenerator(os.path.join(basePath, 'z'), 'w_{theta}_{phi}.jpg'), view, [0,0,0], 100, [0,0,1], [72, 45])
    exp.UpdatePipeline()
    exp = ThreeSixtyImageStackExporter(FileNameGenerator(os.path.join(basePath, 'y'), 'cone_{theta}_{phi}.jpg'), view, [0,0,0], 100, [0,1,0], [72, 45])
    exp.UpdatePipeline()
    exp = ThreeSixtyImageStackExporter(FileNameGenerator(os.path.join(basePath, 'x'), 'cone_{theta}_{phi}.jpg'), view, [0,0,0], 100, [1,0,0], [72, 45])
    exp.UpdatePipeline()
    simple.ResetCamera(view)
    simple.Hide(c, view)
    slice = SliceExplorer(FileNameGenerator(os.path.join(basePath, 'slice'), 'w_{sliceColor}_{slicePosition}.jpg'), view, w, { "RTData": { "lut": lut, "type": 'POINT_DATA'} }, 30, [0,1,0])
    slice.UpdatePipeline()


# -----------------------------------------------------------------------------

def test3(basePath):
    w = simple.Wavelet()
    points_series = [ { "name": "Diagonal" , "probes": [ [ float(x), float(x), float(x) ] for x in range(-10, 10)] }, \
                      { "name": "Slice", "probes": [ [ float(x), float(y), 0.0 ] for x in range(-10, 10) for y in range(-10, 10)] } ]
    time_serie = [ {"name": "Origin", "probe": [0.0, 0.0, 0.0]}, {"name": "FaceCenter", "probe": [10.0, 0.0, 0.0]} ]

    prober = DataProber( FileNameGenerator(os.path.join(basePath, 'dataprober'), 'data_{time}_{field}_{serie}.csv'), w, points_series, [ "RTData" ])
    prober.UpdatePipeline(0.0)

    timeProber = TimeSerieDataProber( FileNameGenerator('/tmp/dataprober_time', 'data_{field}.csv'), w, time_serie, [ "RTData"], 100)
    for time in range(101):
        timeProber.UpdatePipeline(time)

# -----------------------------------------------------------------------------

def testDynamicLighting():

    resolution = 500
    center_of_rotation = [0.0, 0.0, 0.0]
    rotation_axis = [0.0, 0.0, 1.0]
    distance = 150.0

    wavelet = simple.Wavelet()
    wavelet.WholeExtent = [-20,20,-20,20,-20,20]
    filters = [ wavelet ]
    filters_description = [ {'name': 'Wavelet'} ]

    brownianVectors = simple.RandomVectors()

    calculator1 = simple.Calculator()
    calculator1.Function = 'RTData'
    calculator1.ResultArrayName = 'cRTData'

    calculator2 = simple.Calculator()
    calculator2.Function = 'BrownianVectors'
    calculator2.ResultArrayName = 'cBrownianVectors'

    topfilter = calculator2

    simple.UpdatePipeline()

    color_type = [
        ('SOLID_COLOR', [0.8,0.8,0.8])
        ]
    color_by = [ color_type ]

    color_type = [
        #
        #standard composite type fields examples
        #
        #('POINT_DATA', "RTData"),
        #('POINT_DATA', "BrownianVectors"),
        #('CELL_DATA', "cRTData"),
        #('CELL_DATA', "cBrownianVectors"),
        #
        #dynamic rendering composite value type field examples
        #
        ('VALUE', "vRTData"),
        ('VALUE', "vBrownianVectorsX"),
        ('VALUE', "vBrownianVectorsY"),
        ('VALUE', "vBrownianVectorsZ"),
        ('VALUE', "nX"),
        ('VALUE', "nY"),
        ('VALUE', "nZ"),
        ('VALUE', "vRTDataC"),
        ('VALUE', "vBrownianVectorsXC"),
        ('VALUE', "vBrownianVectorsYC"),
        ('VALUE', "vBrownianVectorsZC"),
        ('VALUE', "vnXC"),
        ('VALUE', "vnYC"),
        ('VALUE', "vnZC")
        ]

    pdi = topfilter.GetPointDataInformation()
    cdi = topfilter.GetCellDataInformation()

    luts = {
        #
        #standard composite type fields examples
        #
        #"RTData": simple.GetLookupTableForArray(
        #"RTData", 1, RGBPoints=[
        #     37, 0.23, 0.299, 0.754,
        #    207, 0.865, 0.865, 0.865,
        #     377, 0.706, 0.016, 0.15],
        #VectorMode='Magnitude',
        #NanColor=[0.25, 0.0, 0.0],
        #ColorSpace='Diverging',
        #ScalarRangeInitialized=1.0 ),
        #"BrownianVectors": simple.GetLookupTableForArray(
        #"BrownianVectors", 1, RGBPoints=[
        #    0, 0.23, 0.299, 0.754,
        #    0.5, 0.865, 0.865, 0.865,
        #    1.0, 0.706, 0.016, 0.15],
        #VectorMode='Magnitude',
        #NanColor=[0.25, 0.0, 0.0],
        #ColorSpace='Diverging',
        #ScalarRangeInitialized=1.0 ),
        #"cRTData": simple.GetLookupTableForArray(
        #"cRTData", 1, RGBPoints=[
        #    37, 0.23, 0.299, 0.754,
        #    207, 0.865, 0.865, 0.865,
        #    377, 0.706, 0.016, 0.15],
        #VectorMode='Magnitude',
        #NanColor=[0.25, 0.0, 0.0],
        #ColorSpace='Diverging',
        #ScalarRangeInitialized=1.0 ),
        #"cBrownianVectors": simple.GetLookupTableForArray(
        #"cBrownianVectors", 1, RGBPoints=[
        #    0, 0.23, 0.299, 0.754,
        #    0.5, 0.865, 0.865, 0.865,
        #    1.0, 0.706, 0.016, 0.15],
        #VectorMode='Magnitude',
        #NanColor=[0.25, 0.0, 0.0],
        #ColorSpace='Diverging',
        #ScalarRangeInitialized=1.0 ),
        #
        #dynamic rendering composite value type field examples
        #
        "vRTData": ["point", "RTData", 0, pdi.GetArray("RTData").GetRange()],
        "vBrownianVectorsX": ["point", "BrownianVectors", 0, pdi.GetArray("BrownianVectors").GetRange(0)],
        "vBrownianVectorsY": ["point", "BrownianVectors", 1, pdi.GetArray("BrownianVectors").GetRange(1)],
        "vBrownianVectorsZ": ["point", "BrownianVectors", 2, pdi.GetArray("BrownianVectors").GetRange(2)],
        "nX": ["point", "Normals", 0, (-1,1)],
        "nY": ["point", "Normals", 1, (-1,1)],
        "nZ": ["point", "Normals", 2, (-1,1)],
        "vRTDataC": ["cell", "RTData", 0, pdi.GetArray("RTData").GetRange()],
        "vBrownianVectorsXC": ["cell", "BrownianVectors", 0, pdi.GetArray("BrownianVectors").GetRange(0)],
        "vBrownianVectorsYC": ["cell", "BrownianVectors", 1, pdi.GetArray("BrownianVectors").GetRange(1)],
        "vBrownianVectorsZC": ["cell", "BrownianVectors", 2, pdi.GetArray("BrownianVectors").GetRange(2)],
        "vnXC": ["cell", "Normals", 0, (-1,1)],
        "vnYC": ["cell", "Normals", 1, (-1,1)],
        "vnZC": ["cell", "Normals", 2, (-1,1)]
        }

    contour_values = [ 64.0, 90.6, 117.2, 143.8, 170.4, 197.0, 223.6, 250.2 ]
    contour_values = [ 64.0, 117.2, 170.4, 223.6 ]

    for iso_value in contour_values:
        simple.Contour(
            Input=topfilter,
            PointMergeMethod="Uniform Binning",
            ContourBy = ['POINTS', 'RTData'],
            Isosurfaces = [iso_value],
            ComputeScalars = 1
            )
        #just to get cell aligned normals for testing
        filters.append(
           simple.PointDatatoCellData(PassPointData = 1)
            )
        color_by.append( color_type )
        filters_description.append({'name': 'iso=%s' % str(iso_value)})

    title = "Composite Dynamic Rendering Test"
    description = "A sample file for dynamic rendering"
    analysis = AnalysisManager( '/tmp/wavelet', title, description)

    id = 'composite'
    title = '3D composite'
    description = "contour set"
    analysis.register_analysis(id, title, description, '{theta}/{phi}/{filename}', CompositeImageExporter.get_data_type()+"-light")
    fng = analysis.get_file_name_generator(id)

    camera_handler = ThreeSixtyCameraHandler(
        fng,
        None,
        [ float(r) for r in range(0, 360, 30) ],
        [ float(r) for r in range(-60, 61, 30) ],
        center_of_rotation,
        rotation_axis,
        distance)

    exporter = CompositeImageExporter(
        fng,
        filters,
        color_by,
        luts,
        camera_handler,
        [resolution,resolution],
        filters_description,
        0, 0, 'png')
    exporter.set_analysis(analysis)

    analysis.begin()
    exporter.UpdatePipeline(0)
    analysis.end()
