"""
A module that composites one or more layers together to produce a recognizable
image.
"""

import abc
import numpy as np


class Compositor(object):
    __metaclass__ = abc.ABCMeta
    ''' Base class for different compositor specifications.'''
    def __init__(self):
        self.__bgColor = tuple([0, 0, 0, 0])

    @abc.abstractmethod
    def __renderImpl(self, layers):
        return

    def enableGeometryColor(self, enable):
        # print "Warning: geometry color not supported in this specification."
        return

    def enableLighting(self, enable):
        # print "Warning: lighting not supported in this specification."
        return

    def setColorDefinitions(self, colorDefs):
        return

    def set_background_color(self, rgb):
        self.__bgColor = rgb

    def render(self, layers):
        return self.__renderImpl(layers)


class Compositor_SpecA(Compositor):
    '''
    Compositor SpecA. Loads LayerSpecs consisting of a
    single image. No compositing supported.
    '''
    def __init__(self, parent=None):
        super(Compositor_SpecA, self).__init__()

    def _Compositor__renderImpl(self, layers):
        layer = layers[0] if (len(layers) > 0) else None

        if layer is None:
            raise IndexError("There are no valid layers to render!")

        return layer.getColorArray()


class Compositor_SpecB(Compositor):
    '''
    Compositor SpecB. Composites several LayerSpec instances together.
    Each LayerSpec holds all of its buffer components (depth, color, luminance,
    etc.).
    '''
    def __init__(self, parent=None):
        super(Compositor_SpecB, self).__init__()
        self.__geometryColorEnabled = False
        self.__lightingEnabled = True
        self.__colorDefinitions = {}

    def ambient(self, rgb):
        """ Returns the ambient contribution in an RGB luminance image. """
        return np.dstack((rgb[:, :, 0], rgb[:, :, 0], rgb[:, :, 0]))

    def diffuse(self, rgb):
        """ Returns the diffuse contribution in an RGB luminance image. """
        return np.dstack((rgb[:, :, 1], rgb[:, :, 1], rgb[:, :, 1]))

    def specular(self, rgb):
        """ Returns the specular contribution in an RGB luminance image. """
        return np.dstack((rgb[:, :, 2], rgb[:, :, 2], rgb[:, :, 2]))

    def enableGeometryColor(self, enable):
        self.__geometryColorEnabled = enable

    def enableLighting(self, enable):
        self.__lightingEnabled = enable

    def setColorDefinitions(self, colorDefs):
        self.__colorDefinitions = colorDefs

    def __getCustomizedColorBuffer(self, layer):
        '''
        Queries the user-defined color customizations from a dictionary and
        applies them to a specific LayerSpec instance. The color
        customizations form the ui are matched to its referred LayerSpec using
        the layer name (a 'parameter/value' tag), which is set in the
        QueryTranslator. See the PipelineModel for more information on the
        colorDefinitions struct.
        '''
        array = None
        customizationName = layer.customizationName
        if layer.hasValueArray():
            array = layer.getValueArray()
            valueRange = layer.valueRange

            if customizationName in self.__colorDefinitions:
                array = self.__applyColorLut(
                    array, layer.getDepth(),
                    self.__colorDefinitions[customizationName]["colorLut"],
                    valueRange)

        elif layer.hasColorArray():
            array = np.copy(layer.getColorArray())

            if customizationName in self.__colorDefinitions:
                self.__applyFillColor(
                    array, layer.getDepth(),
                    self.__colorDefinitions[customizationName])

        return array

    def __applyFillColor(self, array, depth, colorDef):
        ''' Applies the user defined geometry color to an array.
        TODO Performance is notoriously affected when calling this function.
        Needs optimization. '''
        if self.__geometryColorEnabled:
            # Process only foreground (object) values
            arrIdx = self.__getForegroundPixels(depth)
            if not arrIdx:
                return
            fill_color = colorDef["geometryColor"]
            array[arrIdx[0], arrIdx[1], :] = fill_color[0:3]

    def __applyColorLut(self, rgbVarr, depth, colorLutStruct, valueRange):
        if colorLutStruct.name == "None":
            return rgbVarr
        # Process only foreground (object) values
        varrIdx = self.__getForegroundPixels(depth)

        if len(rgbVarr.shape) == 2 and rgbVarr.dtype == np.float32:
            if not varrIdx:
                # No foreground. Return a dummy 3c-uint8 image
                # (returning a single chan float image causes issues at
                # the PIL conversion)
                return np.zeros((rgbVarr.shape[0], rgbVarr.shape[1], 3),
                                np.uint8)

            return self.__floatToRGB(
                rgbVarr, varrIdx, colorLutStruct, valueRange)
        else:
            if not varrIdx:
                return rgbVarr

            return self.__invertibleToRGB(rgbVarr, varrIdx, colorLutStruct)

    def __floatToRGB(self, rgbVarr, varrIdx, colorLutStruct, valueRange):
        ''' Decode a value image rendered in FLOATING_POINT mode. The image
        is treated as a plain float buffer, so the color table can be applied
        directly.'''
        # Normalized foreground values down to 0..1 to use as LUT indexes

        # what do we have from raster itself?
        foreground = rgbVarr[varrIdx[0], varrIdx[1]]
        valueMin = foreground.min()
        valueMax = foreground.max()

        if (valueMin >= 0.0) and (valueMax <= 1.0):
            # most likely came from an RGB render, in which case
            # already in 0..1
            # todo: grab from store.metadata.value_mode !=2 instead
            pass
        else:
            # from a direct value render, numbers are actual
            if valueRange:
                # we were given a global range, use it instead of local
                # range from just this slice
                valueMin = valueRange[0]
                valueMax = valueRange[1]
                # this provides a way to clamp, for example NaN's
                foreground[foreground < valueMin] = valueMin
                foreground[foreground > valueMax] = valueMax
            valRange = valueMax - valueMin
            if valRange > 1e-4:
                foreground = (foreground - valueMin) / valRange
            else:
                # no way to know where this single value lies, so use bottom
                foreground.fill(0.0)

        colorLut = colorLutStruct.lut
        bins = colorLutStruct.adjustedBins

        colorIndices = np.digitize(foreground, bins)
        colorIndices = np.subtract(colorIndices, 1)

        shape = rgbVarr.shape
        valueImage = np.zeros([shape[0], shape[1]], dtype=np.uint32)
        valueImage[varrIdx[0], varrIdx[1]] = colorIndices

        return colorLut[valueImage]

    def __invertibleToRGB(self, rgbVarr, varrIdx, colorLutStruct):
        '''
        Decode an RGB image rendered in INVERTIBLE_LUT mode. The image encodes
        float values as colors, so the RGB value is first decoded into its
        represented float value and then the color table is applied.
        '''
        w0 = np.left_shift(
            rgbVarr[varrIdx[0], varrIdx[1], 0].astype(np.uint32), 16)
        w1 = np.left_shift(
            rgbVarr[varrIdx[0], varrIdx[1], 1].astype(np.uint32), 8)
        w2 = rgbVarr[varrIdx[0], varrIdx[1], 2]

        value = np.bitwise_or(w0, w1)
        value = np.bitwise_or(value, w2)
        value = np.subtract(value.astype(np.int32), 1)
        normalized_val = np.divide(value.astype(float), 0xFFFFFE)

        # Map float value to color lut (use a histogram to support non-uniform
        # colormaps (fetch bin indices))
        colorLut = colorLutStruct.lut
        bins = colorLutStruct.adjustedBins

        idx = np.digitize(normalized_val, bins)
        idx = np.subtract(idx, 1)

        valueImage = np.zeros([rgbVarr.shape[0], rgbVarr.shape[1]],
                              dtype=np.uint32)
        valueImage[varrIdx[0], varrIdx[1]] = idx

        return colorLut[valueImage]

    def __getForegroundPixels(self, depth):
        '''
        Computes the indices of the foreground object using the depth buffer.
        '''
        bgDepth = np.max(depth)
        indices = np.where(depth < bgDepth)
        if len(indices[0]) == 0:
            # Only background
            return None
        return indices

    def _Compositor__renderImpl(self, layers):
        """
        Takes an array of layers (LayerSpec) and composites them into an RGB
        image.
        """
        # find a valid LayerSpec (valid = at least one color loaded)
        # necessary for compatibility SpecB-testcase1
        l0 = None
        for index, layer in enumerate(layers):
            if layer.hasColorArray() or layer.hasValueArray():
                # TODO might be necessary to check if the actual array is
                # not NONE too
                l0 = layer
                break

        if l0 is None:
            raise IndexError("There are no valid layers to render!")

        c0 = self.__getCustomizedColorBuffer(l0)

        if self.__lightingEnabled:
            lum0 = l0.getLuminance()
            if lum0 is not None:
                # modulate color of first layer by the luminance
                lum0 = np.copy(lum0)
                lum0 = self.diffuse(lum0)
                c0[:, :, :] = c0[:, :, :] * (lum0[:, :, :]/255.0)

        # composite the rest of the layers
        d0 = np.copy(l0.getDepth())
        for idx in range(1, len(layers)):

            cnext = self.__getCustomizedColorBuffer(layers[idx])
            # necessary for compatibility Spect-testcase1
            if cnext is None:
                continue

            dnext = layers[idx].getDepth()
            lnext = layers[idx].getLuminance()

            # put the top pixels into place
            indices = np.where(dnext < d0)
            if (self.__lightingEnabled and lnext is not None):
                # modulate color by luminance then insert
                lnext = self.diffuse(lnext)
                c0[indices[0], indices[1], :] = \
                    cnext[indices[0], indices[1], :] * \
                    (lnext[indices[0], indices[1], :] / 255.0)
            else:
                # no luminance, direct insert
                c0[indices[0], indices[1], :] = cnext[
                    indices[0], indices[1], :]

            d0[indices[0], indices[1]] = dnext[indices[0], indices[1]]

        if not (d0 is None) and not (d0.ndim == 0):
            # set background pixels to gray to avoid colormap
            # TODO: curious why necessary, we encode a NaN value on these
            # pixels?
            indices = np.where(d0 == np.max(d0))
            __bgColor = self._Compositor__bgColor
            c0[indices[0], indices[1], 0] = __bgColor[0]
            c0[indices[0], indices[1], 1] = __bgColor[1]
            c0[indices[0], indices[1], 2] = __bgColor[2]

        return c0
