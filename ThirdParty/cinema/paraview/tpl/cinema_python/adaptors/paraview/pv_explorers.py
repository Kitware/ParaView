"""
Module consisting of explorers and tracks that connect arbitrary paraview
pipelines to cinema stores.
"""
from __future__ import absolute_import

import numpy as np
import math
from .. import explorers as explorers
from ...images.camera_utils import convert_pose_to_camera

import paraview.simple as simple
from paraview import numpy_support as numpy_support


class ValueMode():
    INVERTIBLE_LUT = 1
    FLOATING_POINT = 2


def printView(view):
    """
    For debugging/demonstrating frustums
    spits out the coordinates of the four camera near plane corners
    load them up in paraview
    """
    r = view.GetRenderer()
    r.SetViewPoint(-1, -1, 0)
    r.ViewToWorld()
    ll = r.GetWorldPoint()
    r.SetViewPoint(1, -1, 0)
    r.ViewToWorld()
    lr = r.GetWorldPoint()
    r.SetViewPoint(1, 1, 0)
    r.ViewToWorld()
    ur = r.GetWorldPoint()
    r.SetViewPoint(-1, 1, 0)
    r.ViewToWorld()
    ul = r.GetWorldPoint()
    print(ll[0], ll[1], ll[2])
    print(lr[0], lr[1], lr[2])
    print(ur[0], ur[1], ur[2])
    print(ul[0], ul[1], ul[2])
    # print (math.sqrt((ur[0]-ll[0])*(ur[0]-ll[0])+
    #                  (ur[1]-ll[1])*(ur[1]-ll[1])+
    #                  (ur[2]-ll[2])*(ur[2]-ll[2])))


class ImageExplorer(explorers.Explorer):
    """
    An Explorer that connects a ParaView script's views to a store.
    Basically it iterates over the parameters and for each unique combination
    it tells ParaView to make an image and saved the result into the store.
    """

    def __init__(self,
                 store, parameters, tracks,
                 view=None,
                 iSave=True):
        super(ImageExplorer, self).__init__(store, parameters, tracks)
        self.view = view
        self.CaptureDepth = False
        self.CaptureLuminance = False
        self.CaptureValues = False
        self.iSave = iSave
        self.UsingGL2 = False
        # Using float rasters for value arrays by default. vtkPVRenderView
        # will fall back to INVERTIBLE_LUT if the required extensions are
        # not supported.
        self.ValueMode = ValueMode().FLOATING_POINT
        self.CheckFloatSupport = True

        if self.view:
            try:
                # TODO We keep LuminancePass purposely off. There is a bug when
                # in which it seems to remain attached even after StopCapture.
                # backend = self.view.GetRenderWindow().GetRenderingBackend()
                # self.UsingGL2 = True if (backend == "OpenGL2") else False
                pass
            except AttributeError:
                pass
        if self.UsingGL2:
            def rgb2grey(rgb, height, width):
                # pass through Diffuse lum term
                as_grey = np.dot(rgb[..., :3], [0.0, 1.0, 0.0])
                res = as_grey.reshape(height, width).astype('uint8')
                return res
            self.rgb2grey = rgb2grey
        else:
            def rgb2grey(rgb, height, width):
                as_grey = np.dot(rgb[..., :3], [0.299, 0.587, 0.114])
                res = as_grey.reshape(height, width).astype('uint8')
                return res
            self.rgb2grey = rgb2grey

    def enableFloatValues(self, enable):
        self.CheckFloatSupport = True if enable else False
        if enable:
            self.ValueMode = ValueMode().FLOATING_POINT
        else:
            self.ValueMode = ValueMode().INVERTIBLE_LUT

    def insert(self, document):
        """overridden to use paraview to generate an image and create a
        the document for it"""
        if not self.view:
            return

        if self.CaptureDepth:
            # Depth capture
            rep = simple.GetRepresentation()
            wasVol = rep.Representation == 'Volume'
            if wasVol:
                rep.Representation = 'Surface'
            simple.Render()
            image = self.view.CaptureDepthBuffer()
            idata = numpy_support.vtk_to_numpy(image) * 256
            rw = self.view.GetRenderWindow()
            width, height = rw.GetSize()
            try:
                imageslice = idata.reshape(height, width)
            except ValueError:
                imageslice = None

            if wasVol:
                rep.Representation = 'Volume'

            document.data = imageslice
            self.CaptureDepth = False
            # printView(self.view)

        else:
            imageslice = None

            if self.CaptureLuminance:
                if self.UsingGL2:
                    # TODO: needs a fix for luminance pass
                    imageslice = self.captureWindowRGB()
                else:  # GL1 support
                    try:
                        rep = simple.GetRepresentation()
                        if rep is not None:
                            rep.DiffuseColor = [1, 1, 1]
                            rep.ColorArrayName = None
                    except ValueError:
                        pass
                    idata, width, height = self.captureWindowAsNumpy()
                    idata = self.rgb2grey(idata, height, width)
                    imageslice = np.dstack((idata, idata, idata))

                document.data = imageslice

            elif self.CaptureValues:  # Value capture
                if self.ValueMode is ValueMode().INVERTIBLE_LUT:
                    imageslice = self.captureWindowRGB()

                elif self.ValueMode is ValueMode().FLOATING_POINT:
                    simple.Render()
                    image = self.view.GetValuesFloat()
                    idata = numpy_support.vtk_to_numpy(image)
                    idataMin = idata.min() if len(idata) > 0 else 0
                    idataMax = idata.max() if len(idata) > 0 else 0
                    self.updateRange(self.view.ArrayNameToDraw,
                                     self.view.ArrayComponentToDraw,
                                     [idataMin, idataMax])
                    rw = self.view.GetRenderWindow()
                    width, height = rw.GetSize()
                    try:
                        imageslice = idata.reshape(height, width)
                    except ValueError:
                        imageslice = None

                document.data = imageslice

            else:  # Capture color image
                imageslice = self.captureWindowRGB()
                document.data = imageslice

        if self.iSave:
            super(ImageExplorer, self).insert(document)

    def captureWindowRGB(self):
        idata, width, height = self.captureWindowAsNumpy()
        imageslice = idata.reshape(height, width, 3)
        return imageslice

    def captureWindowAsNumpy(self):
        image = self.view.CaptureWindow(1)
        ext = image.GetExtent()
        width = ext[1] - ext[0] + 1
        height = ext[3] - ext[2] + 1
        imagescalars = image.GetPointData().GetScalars()
        idata = numpy_support.vtk_to_numpy(imagescalars)
        image.UnRegister(None)
        return idata, width, height

    def setDrawMode(self, choice, **kwargs):
        """ helper for Color tracks so that they can cause ParaView to
        render in the right mode."""
        if choice == 'color':
            self.view.StopCaptureValues()
            if self.UsingGL2:
                self.view.StopCaptureLuminance()
            self.CaptureDepth = False
            self.CaptureLuminance = False
            self.CaptureValues = False
        if choice == 'luminance':
            self.view.StopCaptureValues()
            if self.UsingGL2:
                self.view.StartCaptureLuminance()
            self.CaptureDepth = False
            self.CaptureLuminance = True
            self.CaptureValues = False
        if choice == 'depth':
            self.view.StopCaptureValues()
            if self.UsingGL2:
                self.view.StopCaptureLuminance()
            self.CaptureDepth = True
            self.CaptureLuminance = False
            self.CaptureValues = False
        if choice == 'value':
            if self.UsingGL2:
                self.view.StopCaptureLuminance()

            if self.CaptureValues is not True:
                # this is a workaround for a bug in VTK
                # in valuepass, we need the arrays we will render,
                # but with CompositePolyDataMapper we need to render
                # to find out what arrays are needed
                self.view.StopCaptureValues()
                simple.Render()

            self.view.DrawCells = kwargs['field']
            self.view.ArrayNameToDraw = kwargs['name']
            self.view.ArrayComponentToDraw = kwargs['component']
            self.view.ScalarRange = kwargs['range']

            self.view.StartCaptureValues()
            self.view.SetValueRenderingMode(self.ValueMode)
            # Ensure context support
            if (self.CheckFloatSupport and
                    self.ValueMode == ValueMode().FLOATING_POINT):
                # Force a proxy update to make sure the rendering mode is set
                self.view.UpdateVTKObjects()
                simple.Render()
                self.ValueMode = self.view.GetValueRenderingMode()
                self.CheckFloatSupport = False
                self.store.add_metadata(
                    {'value_mode': self.ValueMode})

            self.CaptureDepth = False
            self.CaptureLuminance = False
            self.CaptureValues = True

    def finish(self):
        super(ImageExplorer, self).finish()
        # TODO: actually record state in init and restore here, for now just
        # make an assumption
        self.view.StopCaptureValues()
        if self.UsingGL2:
            self.view.StopCaptureLuminance()
        try:
            simple.Show()
            simple.Render()
        except RuntimeError:
            pass

    def updateRange(self, name, component, drange):
        fname = name + "_" + str(component)
        plist = self.store.parameter_list
        for pname, param in plist.items():
            if 'valueRanges' in param:
                # now we know it is a color type parameter
                vrange = param['valueRanges'][fname]
                lrange = list(vrange)
                updated = False
                if drange[0] < vrange[0]:
                    updated = True
                    lrange[0] = drange[0]
                if drange[1] > vrange[1]:
                    updated = True
                    lrange[1] = drange[1]
                if updated:
                    param['valueRanges'][fname][0] = float(lrange[0])
                    param['valueRanges'][fname][1] = float(lrange[1])


class Camera(explorers.Track):
    """
    A track that connects a ParaView script's camera to the phi and theta
    tracks. This allows the creation of spherical camera stores where the
    user can view the data from many points around it.
    """
    def __init__(self, center, axis, distance, view):
        super(Camera, self).__init__()
        try:
            # Z => 0 | Y => 2 | X => 1
            self.offset = (axis.index(1) + 1) % 3
        except ValueError:
            raise Exception("Rotation axis not supported", axis)
        self.center = center
        self.distance = distance
        self.view = view

    def execute(self, document):
        """moves camera into position for the current phi, theta value"""
        theta = document.descriptor['theta']
        phi = document.descriptor['phi']

        theta_rad = float(theta) / 180.0 * math.pi
        phi_rad = float(phi) / 180.0 * math.pi
        pos = [
            math.cos(phi_rad) * self.distance * math.cos(theta_rad),
            math.sin(phi_rad) * self.distance * math.cos(theta_rad),
            math.sin(theta_rad) * self.distance
            ]
        up = [
            + math.cos(phi_rad) * math.sin(theta_rad),
            - math.sin(phi_rad) * math.sin(theta_rad),
            + math.cos(theta_rad)
            ]
        for i in range(self.offset):
            pos.insert(0, pos.pop())
            up.insert(0, up.pop())
        pos[0] = float(self.center[0]) - pos[0]
        pos[1] = float(self.center[1]) + pos[1]
        pos[2] = float(self.center[2]) + pos[2]

        self.view.CameraPosition = pos
        self.view.CameraViewUp = up
        self.view.CameraFocalPoint = self.center


class PoseCamera(explorers.Track):
    """
    A track that connects a ParaView script's pose track which has 3x3
    camera orientations in it. Also takes in camera_eye, _at and _up
    arrays, which designate the initial position at each timestep, from
    which the camera moves according to each pose.
    """
    def __init__(self, view, camType, store):
        super(PoseCamera, self).__init__()
        self.view = view
        self.camType = camType
        self.store = store

    def execute(self, document):
        """moves camera into position for the current phi, theta value"""
        pose = document.descriptor['pose']

        # at every timestep paraview records camera location
        # use that as point to spin around
        iPosition = self.store.metadata['camera_eye'][-1]
        iFocalPoint = self.store.metadata['camera_at'][-1]
        iViewUp = self.store.metadata['camera_up'][-1]

        newp, newf, newv = convert_pose_to_camera(
            iPosition, iFocalPoint, iViewUp, pose, self.camType)
        self.view.GetActiveCamera().SetPosition(newp)
        self.view.GetActiveCamera().SetFocalPoint(newf)
        self.view.GetActiveCamera().SetViewUp(newv)


class Slice(explorers.Track):
    """
    A track that connects a slice filter to a scalar valued parameter.
    """
    def __init__(self, parameter, filt):
        super(Slice, self).__init__()

        self.parameter = parameter
        self.slice = filt

    def prepare(self, explorer):
        super(Slice, self).prepare(explorer)

    def execute(self, doc):
        if self.parameter in doc.descriptor:
            o = doc.descriptor[self.parameter]
            self.slice.SliceOffsetValues = [o]


class Contour(explorers.Track):
    """
    A track that connects a contour filter to a scalar valued parameter.
    """
    def __init__(self, parameter, filt):
        super(Contour, self).__init__()
        self.parameter = parameter
        self.contour = filt
        self.control = 'Isosurfaces'

    def prepare(self, explorer):
        super(Contour, self).prepare(explorer)

    def execute(self, doc):
        if self.parameter in doc.descriptor:
            o = doc.descriptor[self.parameter]
            self.contour.SetPropertyWithName(self.control, [o])


class Clip(explorers.Track):
    """
    A track that connects a clip filter to a scalar valued parameter.
    """
    def __init__(self, argument, clip):
        super(Clip, self).__init__()
        self.argument = argument
        self.clip = clip

    def prepare(self, explorer):
        super(Clip, self).prepare(explorer)

    def execute(self, doc):
        if self.argument in doc.descriptor:
            o = doc.descriptor[self.argument]
            self.clip.UseValueAsOffset = True
            self.clip.Value = o


class Templated(explorers.Track):
    """
    A track that connects any type of filter to a scalar valued
    parameter. To use pass in a source proxy (aka filter)
    and the name of method (aka property) to be called on it.
    """
    def __init__(self, parameter, filt, methodName):
        explorers.Track.__init__(self)

        self.parameter = parameter
        self.filt = filt
        self.methodName = methodName

    def execute(self, doc):
        o = doc.descriptor[self.parameter]
        self.filt.SetPropertyWithName(self.methodName, [o])


class ColorList():
    """
    A helper that creates a dictionary of color controls for ParaView.
    The Color track takes in a color name from the Explorer and looks
    up into a ColorList to determine exactly what needs to be set to
    apply the color.
    """
    def __init__(self):
        self._dict = {}

    def AddSolidColor(self, name, RGB):
        self._dict[name] = {'type': 'rgb', 'content': RGB}

    def AddLUT(self, field, name, comp):
        self._dict[name] = {'type': 'lut', 'field': field,
                            'name': name, 'comp': comp}

    def AddDepth(self, name):
        self._dict[name] = {'type': 'depth'}

    def AddLuminance(self, name):
        self._dict[name] = {'type': 'luminance'}

    def AddValueRender(self, name, field, arrayname, component, range):
        self._dict[name] = {'type': 'value',
                            'field': field,
                            'arrayname': arrayname,
                            'component': component,
                            'range': range}

    def getColor(self, name):
        if name in self._dict:
            return self._dict[name]
        else:
            return {'type': 'none'}


class Color(explorers.Track):
    """
    A track that connects a parameter to color controls.
    """
    def __init__(self, parameter, colorlist, rep):
        super(Color, self).__init__()
        self.parameter = parameter
        self.colorlist = colorlist
        self.rep = rep
        self.imageExplorer = None

    def execute(self, doc):
        """tells ParaView to color the object we've been assigned
        using the color definition we've been given that corresponds
        to the value we've been assigned to watch in the doc.descriptor"""
        if self.parameter not in doc.descriptor:
            return
        if self.rep is None:
            return
        o = doc.descriptor[self.parameter]
        spec = self.colorlist.getColor(o)
        if spec['type'] == 'rgb':
            self.rep.DiffuseColor = spec['content']
            self.rep.ColorArrayName = None
            if self.imageExplorer:
                self.imageExplorer.setDrawMode('color')
        if spec['type'] == 'lut':
            aname = spec['name']
            aname = aname[0:aname.rfind("_")]
            simple.ColorBy(self.rep, (spec['field'], aname, spec['comp']))
            if self.imageExplorer:
                self.imageExplorer.setDrawMode('color')
        if spec['type'] == 'depth':
            if self.imageExplorer:
                self.imageExplorer.setDrawMode('depth')
        if spec['type'] == 'luminance':
            if self.imageExplorer:
                self.imageExplorer.setDrawMode('luminance')
        if spec['type'] == 'value':
            if self.imageExplorer:
                self.imageExplorer.setDrawMode("value",
                                               field=spec['field'],
                                               name=spec['arrayname'],
                                               component=spec['component'],
                                               range=spec['range'])


class SourceProxyInLayer(explorers.LayerControl):
    """
    A track that turns on and off an source proxy in a layer
    """
    def showme(self):
        simple.SetActiveSource(self.proxy)
        self.representation.Visibility = 1

    def hideme(self):
        self.representation.Visibility = 0

    def __init__(self, parameter, representation, proxy):
        super(SourceProxyInLayer, self).__init__(
            parameter, self.showme, self.hideme)
        self.representation = representation
        self.proxy = proxy
