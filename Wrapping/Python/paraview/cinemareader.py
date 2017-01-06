r"""cinemareader is used by vtkCinemaDatabaseReader.
"""
from __future__ import print_function

from cinema_python.cinema_store import FileStore
from cinema_python.QueryTranslator_SpecB import QueryTranslator_SpecB
from paraview import vtk
from paraview.vtk.numpy_interface import dataset_adapter as dsa
from cinema_python.utils import convert_pose_to_camera

from ast import literal_eval

def layer2img(layer):
    """converts a layer to vtkImageData."""
    if not layer:
        return None

    img = vtk.vtkImageData()
    dims = [0, 0, 0]
    if layer.hasValueArray():
        nvalues = layer.getValueArray()
        # now, the numpy array's shape matches the 2D image
        dims[0] = nvalues.shape[1]
        dims[1] = nvalues.shape[0]
        img.SetDimensions(dims[0], dims[1], 1)

        nvalues = nvalues.reshape(dims[0]*dims[1])
        vvalues = dsa.numpyTovtkDataArray(nvalues, "Values")
        img.GetPointData().SetScalars(vvalues)

    elif layer.hasColorArray():
        ncolors = layer.getColorArray()
        # now, the numpy array's shape matches the 2D image
        dims[0] = ncolors.shape[1]
        dims[1] = ncolors.shape[0]
        img.SetDimensions(dims[0], dims[1], 1)
        ncolors = ncolors.reshape((dims[0]*dims[1],-1))
        vcolors = dsa.numpyTovtkDataArray(ncolors, "Colors")
        img.GetPointData().SetScalars(vcolors)

    ndepth = layer.getDepth()
    if ndepth is None:
        raise RuntimeError("Missing 'depth'")
    ndepth = ndepth.reshape(dims[0]*dims[1])
    vdepth = dsa.numpyTovtkDataArray(ndepth, "Depth");
    img.GetPointData().AddArray(vdepth)

    nluminance = layer.getLuminance()
    if nluminance is not None:
        nluminance = nluminance.reshape((dims[0]*dims[1], -1))
        vluminance = dsa.numpyTovtkDataArray(nluminance, "Luminance")
        img.GetPointData().AddArray(vluminance)

    #from paraview.vtk.vtkIOLegacy import vtkDataSetWriter
    #writer = vtkDataSetWriter()
    #writer.SetInputDataObject(img)
    #writer.SetFileName("/tmp/layer.vtk")
    #writer.Update()
    #del writer
    return img

class FileStoreAPI(object):
    def __init__(self, fs, qt):
        self.fs = fs
        self.qt = qt
        self.qt.setStore(self.fs)

    def get_objects(self):
        """returns a list of viewable objects in the scene"""
        return []

    def get_parents(self, objectname):
        """returns a list of names of the objects that the given object's
        parents"""
        return []

    def get_visibility(self, object):
        """returns an objects default visbility"""
        return False

    def get_timesteps(self):
        """returns a list of timesteps available in the store"""
        return []

    def get_control_parameters(self, objectname):
        """returns control paramater names for the given object"""
        return []

    def get_control_values(self, controlparametername):
        """returns values available for a given control parameter"""
        return []

    def get_control_values_as_strings(self, controlparametername):
        vals = self.get_control_values(controlparametername)
        return [str(x) for x in vals]

    def get_field_name(self, objectname):
        """return the field name associated with an object"""
        return None

    def get_field_values(self, objectname, valuetype):
        """returns fields for a specific object"""
        return []

    def get_field_valuerange(self, objectname, valueanme):
        return []

    def translate_query(self, q):
        if type(q) == str:
            q = literal_eval(q)
        assert type(q) == dict

        # update pose
        poseIndex = q.get("pose", -1)
        if type(poseIndex) == int:
            if poseIndex != -1:
                q["pose"] = self.fs.get_parameter("pose")["values"][poseIndex]
            else:
                return []

        #print "query", q
        layers = self.qt.translateQuery(q)

        # convert layers to image data
        return [layer2img(l) for l in layers]


class FileStoreSpecB(FileStoreAPI):
    def __init__(self, fs):
        FileStoreAPI.__init__(self, fs, QueryTranslator_SpecB())
        self.__cameras = {}

    def get_objects(self):
        # to make our lives much easier, we returns the objects in a sorted
        # order from upstream to downstream in the pipeline.
        objs = self.fs.parameter_list["vis"]["values"]
        retval = []

        all_parents = self.get_all_parents()

        def _add_object(me, obj, lst):
            if obj in lst:
                return
            parents = all_parents.get(obj,[])
            for p in parents:
                _add_object(me, p, lst)
            lst.append(obj)

        for o in objs:
            _add_object(self, o, retval)
        return retval

    def get_all_parents(self):
        """builds a map with key is object name, and value is a list of
        objectname for its parents."""
        md = self.fs.metadata
        pipeline = md.get("pipeline", [])
        idmap = {}
        for item in pipeline:
            idmap[item["id"]] = item["name"]

        parents = {}
        for item in pipeline:
            itemparents = []
            for pid in item.get("parents",[]):
                if idmap.has_key(pid):
                    itemparents.append(idmap[pid])
            parents[item["name"]] = itemparents
        return parents

    def get_parents(self, objectname):
        ps = self.get_all_parents()
        return ps.get(objectname, [])

    def get_visibility(self, objectname):
        md = self.fs.metadata
        pipeline = md.get("pipeline", [])
        for item in pipeline:
            if item["name"] == objectname:
                return item.get("visibility",False) == True
        return False

    def get_control_parameters(self, objectname):
        (tmp0, tmp1, controls) = self.fs.parameters_for_object(objectname)
        # I am going to prune any control parameters that are not the parameter
        # itself. This removes control parameters coming from up the pipeline.
        if objectname in controls:
            return [objectname]
        else:
            return []

    def get_control_values(self, controlparametername):
        """returns values available for a given control parameter"""
        info = self.fs.get_parameter(controlparametername)
        return info["values"]

    def get_field_name(self, objectname):
        (tmp0, field, tmp1) = self.fs.parameters_for_object(objectname)
        return field

    def get_field_values(self, objectname, valuetype):
        """returns fields for a specific object"""
        (tmp0, field, tmp1) = self.fs.parameters_for_object(objectname)
        if not field:
            return []
        retval = []
        p = self.fs.get_parameter(field)
        for (n,t) in zip(p["values"], p["types"]):
            if t == valuetype:
                retval.append(n)
        return retval

    def get_field_valuerange(self, objectname, valuename):
        (tmp0, field, tmp1) = self.fs.parameters_for_object(objectname)
        if not field:
            return []
        p = self.fs.get_parameter(field)
        if p and p["valueRanges"].has_key(valuename):
            return list(p["valueRanges"][valuename])
        return []

    def get_timesteps(self):
        """returns a list of timesteps available in the store"""
        if "time" in self.fs.parameter_list:
            return list(self.get_control_values("time"))
        return []

    def get_cameras(self, ts):
        """returns a list of camera positions for a specific timestep. timestep
        is ignored if camera is not affected by time
        """
        try:
            tsindex = self.get_timesteps().index(ts)
        except ValueError:
            # no timesteps or invalid timestep.
            assert len(self.get_timesteps()) == 0
            tsindex = 0

        # use cached cameras, if available.
        if self.__cameras.has_key(tsindex):
            return self.__cameras[tsindex]

        # if not cached, read and cache.
        cameras = []
        md = self.fs.metadata
        camera_model = md["camera_model"]
        if (camera_model == "azimuth-elevation-roll") or (camera_model == "yaw-pitch-roll"):
            eye = md["camera_eye"][tsindex][:3]
            at  = md["camera_at"][tsindex][:3]
            up = md["camera_up"][tsindex][:3]
            nearfar = md["camera_nearfar"][tsindex][:2]
            angle = md["camera_angle"][tsindex]
            poses = self.fs.get_parameter("pose")["values"]
            for pose in poses:
                neweye, newat, newup = convert_pose_to_camera(eye, at, up, pose, camera_model)
                camera = vtk.vtkCamera()
                camera.SetPosition(neweye)
                camera.SetFocalPoint(newat)
                camera.SetViewUp(newup)
                camera.SetViewAngle(angle)
                camera.SetClippingRange(nearfar[0], nearfar[1])
                cameras.append(camera)
        self.__cameras[tsindex] = cameras
        return cameras

__warning_count = {}
def load(filename):
    global __warning_count
    fs = FileStore(filename)
    fs.load()

    # check if we support this cinema database.
    if fs.metadata.get("type") != "composite-image-stack":
        raise RuntimeError("Only 'composite-image-stack' file stores are supported (aka Spec C)")
    if fs.metadata.get("value_mode") != 2:
        if not __warning_count.has_key(filename):
            __warning_count[filename] = True
            print ("Warning: the cinema store", filename, ", encodes data values as RGB arrays "\
            "which is known to have issues in current implementation. Scalaring "\
            "coloring will produce unexpected results.")
    return FileStoreSpecB(fs)
