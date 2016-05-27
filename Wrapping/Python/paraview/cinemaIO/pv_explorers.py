#==============================================================================
# Copyright (c) 2015,  Kitware Inc., Los Alamos National Laboratory
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this
# list of conditions and the following disclaimer in the documentation and/or other
# materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors may
# be used to endorse or promote products derived from this software without specific
# prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
# OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#==============================================================================
"""
    Module consisting of explorers and tracks that connect arbitrary paraview
    pipelines to cinema stores.
"""

import explorers
import paraview.simple as simple
import numpy as np
import paraview
from paraview import numpy_support as numpy_support

class ImageExplorer(explorers.Explorer):
    """
    An Explorer that connects a ParaView script's views to a store.
    Basically it iterates over the parameters and for each unique combination
    it tells ParaView to make an image and saved the result into the store.
    """
    def __init__(self,
                cinema_store, parameters, tracks,
                view=None,
                iSave=True):
        super(ImageExplorer, self).__init__(cinema_store, parameters, tracks)
        self.view = view
        self.CaptureDepth = False
        self.CaptureLuminance = False
        self.iSave = iSave
        self.UsingGL2 = False
        if self.view:
            try:
                rw=self.view.GetRenderWindow()
                if rw.GetRenderingBackend()==2:
                    self.UsingGL2 = True
            except AttributeError:
                pass
        if self.UsingGL2:
            def rgb2grey(rgb, height, width):
                as_grey = np.dot(rgb[...,:3], [0.0, 1.0, 0.0]) #pass through Diffuse lum term
                res = as_grey.reshape(height,width).astype('uint8')
                return res
            self.rgb2grey = rgb2grey
        else:
            def rgb2grey(rgb, height, width):
                as_grey = np.dot(rgb[...,:3], [0.299, 0.587, 0.144])
                res = as_grey.reshape(height,width).astype('uint8')
                return res
            self.rgb2grey = rgb2grey

    def insert(self, document):
        """overridden to use paraview to generate an image and create a
        the document for it"""
        if not self.view:
            return
        if self.CaptureDepth:
            simple.Render()
            image = self.view.CaptureDepthBuffer()
            idata = numpy_support.vtk_to_numpy(image) * 256
            rw = self.view.GetRenderWindow()
            width,height = rw.GetSize()
            try:
                imageslice = idata.reshape(height,width)
            except ValueError:
                imageslice = None
            #import Image
            #img = Image.fromarray(imageslice)
            #img.show()
            #try:
            #    input("Press enter to continue ")
            #except NameError:
            #    pass
            document.data = imageslice
            self.CaptureDepth = False
        else:
            imageslice = None
            if self.CaptureLuminance and not self.UsingGL2:
                try:
                    rep = simple.GetRepresentation()
                    if rep != None:
                        rep.DiffuseColor = [1,1,1]
                        rep.ColorArrayName = None
                except ValueError:
                    pass
                image = self.view.CaptureWindow(1)
                ext = image.GetExtent()
                width = ext[1] - ext[0] + 1
                height = ext[3] - ext[2] + 1
                imagescalars = image.GetPointData().GetScalars()
                idata = numpy_support.vtk_to_numpy(imagescalars)
                idata = self.rgb2grey(idata, height, width)
                imageslice = np.dstack((idata,idata,idata))
                image.UnRegister(None)
            else:
                image = self.view.CaptureWindow(1)
                ext = image.GetExtent()
                width = ext[1] - ext[0] + 1
                height = ext[3] - ext[2] + 1
                imagescalars = image.GetPointData().GetScalars()
                idata = numpy_support.vtk_to_numpy(imagescalars)
                imageslice = idata.reshape(height,width,3)
                image.UnRegister(None)
            #import Image
            #img = Image.fromarray(imageslice)
            #img.show()
            #try:
            #    input("Press enter to continue ")
            #except NameError:
            #    pass
            document.data = imageslice
        if self.iSave:
            super(ImageExplorer, self).insert(document)

    def setDrawMode(self, choice, **kwargs):
        """ helper for Color tracks so that they can cause ParaView to
        render in the right mode."""
        if choice == 'color':
            self.view.StopCaptureValues()
            if self.UsingGL2:
                self.view.StopCaptureLuminance()
            self.CaptureDepth = False
            self.CaptureLuminance = False
        if choice == 'luminance':
            self.view.StopCaptureValues()
            if self.UsingGL2:
                self.view.StartCaptureLuminance()
            self.CaptureDepth = False
            self.CaptureLuminance = True
        if choice == 'depth':
            self.view.StopCaptureValues()
            if self.UsingGL2:
                self.view.StopCaptureLuminance()
            self.CaptureDepth=True
            self.CaptureLuminance = False
        if choice == 'value':
            if self.UsingGL2:
                self.view.StopCaptureLuminance()
            self.view.DrawCells = kwargs['field']
            self.view.ArrayNameToDraw = kwargs['name']
            self.view.ArrayComponentToDraw = kwargs['component']
            self.view.ScalarRange = kwargs['range']
            self.view.StartCaptureValues()
            self.CaptureDepth = False
            self.CaptureLuminance = False

    def finish(self):
        super(ImageExplorer, self).finish()
        #TODO: actually record state in init and restore here, for now just
        #make an assumption
        self.view.StopCaptureValues()
        if self.UsingGL2:
            self.view.StopCaptureLuminance()
        try:
            simple.Show()
            simple.Render()
        except RuntimeError:
            pass

class Camera(explorers.Track):
    """
    A track that connects a ParaView script's camera to the phi and theta tracks.
    This allows the creation of spherical camera stores where the user can
    view the data from many points around it.
    """
    def __init__(self, center, axis, distance, view):
        super(Camera, self).__init__()
        try:
            # Z => 0 | Y => 2 | X => 1
            self.offset = (axis.index(1) + 1 ) % 3
        except ValueError:
            raise Exception("Rotation axis not supported", axis)
        self.center = center
        self.distance = distance
        self.view = view

    def execute(self, document):
        """moves camera into position for the current phi, theta value"""
        import math
        theta = document.descriptor['theta']
        phi = document.descriptor['phi']

        theta_rad = float(theta) / 180.0 * math.pi
        phi_rad = float(phi) / 180.0 * math.pi
        pos = [
            float(self.center[0]) - math.cos(phi_rad)   * self.distance * math.cos(theta_rad),
            float(self.center[1]) + math.sin(phi_rad)   * self.distance * math.cos(theta_rad),
            float(self.center[2]) + math.sin(theta_rad) * self.distance
            ]
        up = [
            + math.cos(phi_rad) * math.sin(theta_rad),
            - math.sin(phi_rad) * math.sin(theta_rad),
            + math.cos(theta_rad)
            ]
        self.view.CameraPosition = pos
        self.view.CameraViewUp = up
        self.view.CameraFocalPoint = self.center

    @staticmethod
    def obtain_angles(angular_steps=[10,15]):
        import math
        thetas = []
        phis = []
        theta_offset = 90 % angular_steps[1]
        if theta_offset == 0:
            theta_offset += angular_steps[1]
        for theta in range(-90 + theta_offset,
                           90 - theta_offset + 1, angular_steps[1]):
            theta_rad = float(theta) / 180.0 * math.pi
            for phi in range(0, 360, angular_steps[0]):
                phi_rad = float(phi) / 180.0 * math.pi
                thetas.append(theta)
                phis.append(phi)
        return thetas, phis

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
            self.slice.SliceOffsetValues=[o]

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
            self.contour.SetPropertyWithName(self.control,[o])

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
        self.filt.SetPropertyWithName(self.methodName,[o])

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
        self._dict[name] = {'type':'rgb','content':RGB}

    def AddLUT(self, name, lut):
        self._dict[name] = {'type':'lut','content':lut}

    def AddDepth(self, name):
        self._dict[name] = {'type':'depth'}

    def AddLuminance(self, name):
        self._dict[name] = {'type':'luminance'}

    def AddValueRender(self, name, field, arrayname, component, range):
        self._dict[name] = {'type':'value',
                            'field':field,
                            'arrayname':arrayname,
                            'component':component,
                            'range':range}

    def getColor(self, name):
        return self._dict[name]

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
        if not self.parameter in doc.descriptor:
            return
        if self.rep == None:
            #TODO: probably a bad sign
            return
        o = doc.descriptor[self.parameter]
        spec = self.colorlist.getColor(o)
        found = False
        if spec['type'] == 'rgb':
            found = True
            self.rep.DiffuseColor = spec['content']
            self.rep.ColorArrayName = None
            if self.imageExplorer:
                self.imageExplorer.setDrawMode('color')
        if spec['type'] == 'lut':
            found = True
            self.rep.LookupTable = spec['content']
            self.rep.ColorArrayName = o
            if self.imageExplorer:
                self.imageExplorer.setDrawMode('color')
        if spec['type'] == 'depth':
            found = True
            if self.imageExplorer:
                self.imageExplorer.setDrawMode('depth')
        if spec['type'] == 'luminance':
            found = True
            if self.imageExplorer:
                self.imageExplorer.setDrawMode('luminance')
        if spec['type'] == 'value':
            found = True
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
        self.representation.Visibility = 1

    def hideme(self):
        self.representation.Visibility = 0

    def __init__(self, parameter, representation):
        super(SourceProxyInLayer, self).__init__(parameter, self.showme, self.hideme)
        self.representation = representation
