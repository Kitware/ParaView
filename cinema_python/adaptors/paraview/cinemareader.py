r"""cinemareader is used by vtkCinemaDatabaseReader.
"""
from __future__ import absolute_import, print_function

from ...database import file_store as file_store
from ...images import querymaker
from ...images import querymaker_specb as CT
from ...images.camera_utils import convert_pose_to_camera

from paraview import vtk
from paraview.vtk.numpy_interface import dataset_adapter as dsa

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
        ncolors = ncolors.reshape((dims[0]*dims[1], -1))
        vcolors = dsa.numpyTovtkDataArray(ncolors, "Colors")
        img.GetPointData().SetScalars(vcolors)

    ndepth = layer.getDepth()
    if ndepth is None:
        raise RuntimeError("Missing 'depth'")
    ndepth = ndepth.reshape(dims[0]*dims[1])
    vdepth = dsa.numpyTovtkDataArray(ndepth, "Depth")
    img.GetPointData().AddArray(vdepth)

    nluminance = layer.getLuminance()
    if nluminance is not None:
        nluminance = nluminance.reshape((dims[0]*dims[1], -1))
        vluminance = dsa.numpyTovtkDataArray(nluminance, "Luminance")
        img.GetPointData().AddArray(vluminance)

    # from paraview.vtk.vtkIOLegacy import vtkDataSetWriter
    # writer = vtkDataSetWriter()
    # writer.SetInputDataObject(img)
    # writer.SetFileName("/tmp/layer.vtk")
    # writer.Update()
    # del writer
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

    def get_control_values(self, controlparametername, sort=True):
        """returns sorted list of available values for a control parameter"""
        info = self.fs.get_parameter(controlparametername)
        values = info["values"]
        if sort:
            values.sort()
        return values

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
        return []

    def get_spec(self):
        """returns current database spec """
        return ""


class FileStoreSpecA(FileStoreAPI):
    def __init__(self, fs):
        FileStoreAPI.__init__(self, fs, querymaker.QueryMaker_SpecA())
        self.__cameras = []

    def get_objects(self):
        """returns a list of viewable objects in the scene"""
        objects = ["Cinema"]

        return objects

    def get_parents(self, objectname):
        if objectname == "Cinema":
            return []

        return ["Cinema"]

    def get_visibility(self, objectname):
        """returns an objects default visbility"""
        return True

    def get_control_parameters(self, objectname):
        """returns all parameters for this database"""
        params = []
        for param in self.fs.parameter_list.keys():
            params.append(param)

        return params

    def get_cameras(self, ts):
        """returns a list of camera positions timestep is ignored."""

        # use cached cameras, if available.
        if 0 in self.__cameras:
            return self.__cameras

        # if not cached, read and cache.
        md = self.fs.metadata

        try:
            eye = md["camera_eye"][0][:3]
            at = md["camera_at"][0][:3]
            up = md["camera_up"][0][:3]
            nearfar = md["camera_nearfar"][0][:2]
            angle = md["camera_angle"][0]

        except KeyError:
            print("Warning: Cannot initialiaze cameras." +
                  " Interaction may not work correctly")

            return self.__cameras

        phis = [0]
        if "phi" in self.fs.parameter_list.keys():
            phis = self.fs.get_parameter("phi")["values"]
        thetas = [0]
        if "theta" in self.fs.parameter_list.keys():
            thetas = self.fs.get_parameter("theta")["values"]

        camera = vtk.vtkCamera()
        camera.SetPosition(eye)
        camera.SetFocalPoint(at)
        camera.SetViewUp(up)
        camera.SetViewAngle(angle)
        camera.SetClippingRange(nearfar[0], nearfar[1])

        for phi in phis:
            for theta in thetas:
                c = vtk.vtkCamera()
                c.DeepCopy(camera)
                c.Azimuth(phi)
                c.Elevation(theta)
                c.OrthogonalizeViewUp()
                self.__cameras.append(c)

        return self.__cameras

    def translate_query(self, q):
        if type(q) == str:
            q = literal_eval(q)
        assert type(q) == dict

        # be sure all params have a value in the new query
        query = {}
        for param in self.fs.parameter_list.keys():
            default = [self.fs.get_parameter(param)["default"]]
            query[param] = q.get(param, default)

        layers = self.qt.translateQuery(query)

        # convert layers to image data

        layer = layers[0]
        img = vtk.vtkImageData()
        dims = [0, 0, 0]
        if layer.hasColorArray():
            ncolors = layer.getColorArray()
            # now, the numpy array's shape matches the 2D image
            dims[0] = ncolors.shape[1]
            dims[1] = ncolors.shape[0]
            img.SetDimensions(dims[0], dims[1], 1)
            ncolors = ncolors.reshape((dims[0]*dims[1], -1))
            vcolors = dsa.numpyTovtkDataArray(ncolors, "Colors")
            img.GetPointData().SetScalars(vcolors)

        nluminance = layer.getLuminance()
        if nluminance is not None:
            nluminance = nluminance.reshape((dims[0]*dims[1], -1))
            vluminance = dsa.numpyTovtkDataArray(nluminance, "Luminance")
            img.GetPointData().AddArray(vluminance)

        return [img]

    def get_spec(self):
        return "specA"


class FileStoreSpecB(FileStoreAPI):
    def __init__(self, fs):
        FileStoreAPI.__init__(self, fs, CT.QueryMaker_SpecB())
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
            parents = all_parents.get(obj, [])
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
            for pid in item.get("parents", []):
                if pid in idmap:
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
                return item.get("visibility", 0) == 1
        return False

    def get_control_parameters(self, objectname):
        (tmp0, tmp1, controls) = self.fs.parameters_for_object(objectname)
        # I am going to prune any control parameters that are not the parameter
        # itself. This removes control parameters coming from up the pipeline.
        if objectname in controls:
            return [objectname]
        else:
            return []

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
        for (n, t) in zip(p["values"], p["types"]):
            if t == valuetype:
                retval.append(n)
        return retval

    def get_field_valuerange(self, objectname, valuename):
        (tmp0, field, tmp1) = self.fs.parameters_for_object(objectname)
        if not field:
            return []
        p = self.fs.get_parameter(field)
        if p and valuename in p["valueRanges"]:
            return list(p["valueRanges"][valuename])
        return []

    def get_timesteps(self):
        """returns a list of timesteps available in the store"""
        if "time" in self.fs.parameter_list:
            return list(self.get_control_values("time", False))
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
        if tsindex in self.__cameras:
            return self.__cameras[tsindex]

        # if not cached, read and cache.
        cameras = []
        md = self.fs.metadata
        camera_model = md["camera_model"]
        if ((camera_model == "azimuth-elevation-roll") or
                (camera_model == "yaw-pitch-roll")):
            eye = md["camera_eye"][tsindex][:3]
            at = md["camera_at"][tsindex][:3]
            up = md["camera_up"][tsindex][:3]
            nearfar = md["camera_nearfar"][tsindex][:2]
            angle = md["camera_angle"][tsindex]
            poses = self.fs.get_parameter("pose")["values"]
            for pose in poses:
                neweye, newat, newup = convert_pose_to_camera(
                    eye, at, up, pose, camera_model)
                camera = vtk.vtkCamera()
                camera.SetPosition(neweye)
                camera.SetFocalPoint(newat)
                camera.SetViewUp(newup)
                camera.SetViewAngle(angle)
                camera.SetClippingRange(nearfar[0], nearfar[1])
                cameras.append(camera)
        self.__cameras[tsindex] = cameras
        return cameras

    def translate_query(self, q):
        if type(q) == str:
            q = literal_eval(q)
        assert type(q) == dict

        # be sure all params have a value in the new query
        query = {}
        for param in self.fs.parameter_list.keys():
            default = [self.fs.get_parameter(param)["default"]]
            query[param] = q.get(param, default)

        # update pose
        poseIndex = query.get("pose", -1)
        if type(poseIndex) == int:
            if poseIndex != -1:
                query["pose"] = self.fs.get_parameter(
                    "pose")["values"][poseIndex]
            else:
                return []

        # print ("query", q)
        layers = self.qt.translateQuery(query)

        # convert layers to image data
        return [layer2img(l) for l in layers]

    def get_spec(self):
        return "specC"


__warning_count = {}


def load(filename):
    global __warning_count
    fs = file_store.FileStore(filename)
    fs.load()

    if fs.metadata.get("type") == "parametric-image-stack":
        return FileStoreSpecA(fs)

    # check if we support this cinema database.
    if fs.metadata.get("type") != "composite-image-stack":
        print("Only 'composite-image-stack' and 'parametric-image-stack'" +
              " file stores are supported.")
        raise RuntimeError(
            "Only 'composite-image-stack' and 'parametric-image-stack'" +
            " file stores are supported.")
    if fs.metadata.get("camera_model") != "azimuth-elevation-roll":
        print("Only 'azimuth-elevation-roll' cameras are supported.")
        raise RuntimeError(
            "Only 'azimuth-elevation-roll' cameras are supported.")
    if fs.metadata.get("value_mode") != 2:
        if filename not in __warning_count:
            __warning_count[filename] = True
            print("Warning: the cinema store '" + filename.strip() +
                  "', encodes data values as RGB arrays which is known to " +
                  "have issues in current implementation. Scalar" +
                  "coloring may produce unexpected results.")

    return FileStoreSpecB(fs)
