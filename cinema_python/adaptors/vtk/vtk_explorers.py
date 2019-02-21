"""
Module consisting of explorers and tracks that connect arbitrary VTK
pipelines to cinema stores.
"""
from __future__ import absolute_import

from .. import explorers

import vtk
from vtk.numpy_interface import dataset_adapter as dsa
import numpy as np


class ImageExplorer(explorers.Explorer):
    """
    An explorer that connects a VTK program's render window to a store
    and makes it save new images into the store.
    """
    def __init__(self, store, parameters, engines, rw):
        super(ImageExplorer, self).__init__(store, parameters, engines)
        self.rw = rw
        self.w2i = vtk.vtkWindowToImageFilter()
        # self.w2i.ReadFrontBufferOff()
        self.w2i.SetInput(self.rw)
        self.vp = None
        self.lp = None

    def insert(self, document):
        """overridden to use VTK to generate an image and create a
        the document for it"""
        r = self.rw.GetRenderers().GetFirstRenderer()
        r.Clear()  # TODO: shouldn't be necessary
        # import time
        # time.sleep(1)
        self.w2i.Modified()
        self.w2i.Update()
        image = self.w2i.GetOutput()
        npview = dsa.WrapDataObject(image)
        idata = npview.PointData[0]
        ext = image.GetExtent()
        width = ext[1] - ext[0] + 1
        height = ext[3] - ext[2] + 1
        if image.GetNumberOfScalarComponents() == 1:
            imageslice = np.flipud(idata.reshape(height, width))
        else:
            imageslice = np.flipud(
                idata.reshape(
                    height, width, image.GetNumberOfScalarComponents()))
        document.data = imageslice
        super(ImageExplorer, self).insert(document)

    def setDrawMode(self, choice, **kwargs):
        """ helper for Color tracks so that they can cause ParaView to
        render in the right mode."""
        self.rw.GetRenderers().GetFirstRenderer().SetPass(None)
        if choice == 'color':
            self.w2i.SetInputBufferTypeToRGB()
        if choice == 'depth':
            self.w2i.SetInputBufferTypeToZBuffer()
        if choice == 'value':
            self.w2i.SetInputBufferTypeToRGB()
            if not self.vp:
                self.vp = vtk.vtkValuePass()
            self.vp.SetInputArrayToProcess(kwargs['field'], kwargs['name'])
            self.vp.SetInputComponentToProcess(kwargs['component'])
            self.vp.SetScalarRange(kwargs['range'][0], kwargs['range'][1])
            self.rw.GetRenderers().GetFirstRenderer().SetPass(self.vp)
        if choice == 'luminance':
            self.w2i.SetInputBufferTypeToRGB()
            if not self.lp:
                self.lp = vtk.vtkLightingMapPass()
            self.rw.GetRenderers().GetFirstRenderer().SetPass(self.lp)


class Clip(explorers.Track):
    """
    A track that connects clip filters to a scalar valued parameter.
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
            self.clip.SetValue(o)  # <---- the most important thing!


class Contour(explorers.Track):
    """
    A track that connects clip filters to a scalar valued parameter.
    """
    def __init__(self, argument, filter, method):
        super(Contour, self).__init__()
        self.argument = argument
        self.filter = filter
        self.method = method

    def prepare(self, explorer):
        super(Contour, self).prepare(explorer)

    def execute(self, doc):
        if self.argument in doc.descriptor:
            o = doc.descriptor[self.argument]
            getattr(self.filter, self.method)(0, o)
            self.filter.Update()


class ColorList():
    """
    A helper that creates a dictionary of color controls for VTK. The Color
    track takes in a color name from the Explorer and looks up into a
    ColorList to determine exactly what needs to be set to apply the color.
    """
    def __init__(self):
        self._dict = {}

    def AddSolidColor(self, name, RGB):
        self._dict[name] = {'type': 'rgb', 'content': RGB}

    def AddLUT(self, name, lut, field, arrayname):
        self._dict[name] = {
            'type': 'lut', 'content': lut, 'field': field,
            'arrayname': arrayname}

    def AddDepth(self, name):
        self._dict[name] = {'type': 'depth'}

    def AddLuminance(self, name):
        self._dict[name] = {'type': 'luminance'}

    def AddValueRender(self, name, field, arrayname, component, range):
        self._dict[name] = {
            'type': 'value',
            'field': field, 'arrayname': arrayname,
            'component': component, 'range': range}

    def getColor(self, name):
        if name in self._dict:
            return self._dict[name]
        else:
            return {'type': 'none'}


class ColorActors(explorers.Track):
    """
    A track that connects a parameter to a choice of surface rendered color
    maps.
    """
    def __init__(self, parameter, colorlist, actors):
        super(ColorActors, self).__init__()
        self.parameter = parameter
        self.colorlist = colorlist
        self.actors = actors
        self.imageExplorer = None

    def execute(self, doc):
        """tells VTK to color the object we've been assigned
        using the color definition we've been given that corresponds
        to the value we've been assigned to watch in the doc.descriptor"""
        if self.parameter in doc.descriptor:
            o = doc.descriptor[self.parameter]
            spec = self.colorlist.getColor(o)
            if spec['type'] == 'rgb':
                if self.imageExplorer:
                    self.imageExplorer.setDrawMode("color")
                for a in self.actors:
                    a.GetMapper().ScalarVisibilityOff()
                    a.GetProperty().SetColor(spec['content'])
            if spec['type'] == 'lut':
                if self.imageExplorer:
                    self.imageExplorer.setDrawMode("color")
                for a in self.actors:
                    a.GetMapper().ScalarVisibilityOn()
                    a.GetMapper().SetLookupTable(spec['content'])
                    a.GetMapper().SetScalarMode(spec['field'])
                    a.GetMapper().SelectColorArray(spec['arrayname'])
            if spec['type'] == 'depth':
                if self.imageExplorer:
                    self.imageExplorer.setDrawMode("depth")
                for a in self.actors:
                    a.GetMapper().ScalarVisibilityOff()
            if spec['type'] == 'luminance':
                if self.imageExplorer:
                    self.imageExplorer.setDrawMode('luminance')
                for a in self.actors:
                    a.GetMapper().ScalarVisibilityOff()
            if spec['type'] == 'value':
                if self.imageExplorer:
                    self.imageExplorer.setDrawMode("value",
                                                   field=spec['field'],
                                                   name=spec['arrayname'],
                                                   component=spec['component'],
                                                   range=spec['range'])
                for a in self.actors:
                    a.GetMapper().ScalarVisibilityOn()


class Color(ColorActors):
    def __init__(self, parameter, colorlist, actor):
        super(Color, self).__init__(parameter, colorlist, [actor])


class ActorInLayer(explorers.LayerControl):
    """
    A track that turns on and off an actor in a layer
    """
    def showme(self):
        # print self.name, "\tON"
        self.actor.VisibilityOn()

    def hideme(self):
        # print self.name, "\tOFF"
        self.actor.VisibilityOff()

    def __init__(self, parameter, actor):
        super(ActorInLayer, self).__init__(parameter, self.showme, self.hideme)
        self.actor = actor


class Camera(explorers.Track):
    """
    A track that connects a VTK script's camera to the phi and theta tracks.
    This allows the creation of spherical camera stores where the user can
    view the data from many points around it.
    """
    def __init__(self, center, axis, distance, camera):
        super(Camera, self).__init__()
        try:
            # Z => 0 | Y => 2 | X => 1
            self.offset = (axis.index(1) + 1) % 3
        except ValueError:
            raise Exception("Rotation axis not supported", axis)
        self.center = center
        self.distance = distance
        self.camera = camera

    def execute(self, document):
        import math
        theta = document.descriptor['theta']
        phi = document.descriptor['phi']

        theta_rad = float(theta) / 180.0 * math.pi
        phi_rad = float(phi) / 180.0 * math.pi
        pos = [
            float(self.center[0]) - (
                math.cos(phi_rad) * self.distance * math.cos(theta_rad)),
            float(self.center[1]) + (
                math.sin(phi_rad) * self.distance * math.cos(theta_rad)),
            float(self.center[2]) + math.sin(theta_rad) * self.distance
            ]
        up = [
            + math.cos(phi_rad) * math.sin(theta_rad),
            - math.sin(phi_rad) * math.sin(theta_rad),
            + math.cos(theta_rad)
            ]
        self.camera.SetPosition(pos)
        self.camera.SetViewUp(up)
        self.camera.SetFocalPoint(self.center)

    @staticmethod
    def obtain_angles(angular_steps=[10, 15]):
        thetas = []
        phis = []
        theta_offset = 90 % angular_steps[1]
        if theta_offset == 0:
            theta_offset += angular_steps[1]
        for theta in range(-90 + theta_offset,
                           90 - theta_offset + 1, angular_steps[1]):
            for phi in range(0, 360, angular_steps[0]):
                thetas.append(theta)
                phis.append(phi)
        return thetas, phis
