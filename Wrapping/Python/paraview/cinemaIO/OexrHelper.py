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
Interface to OpenEXR library.
TODO: fold this into raster_wrangler.
"""

import OpenEXR as oe
import Imath as im
import numpy as np

class OexrCompression:
    NONE = 0
    RLE = 1
    ZIPS = 2
    ZIP = 3
    PIZ = 4
    PXR24 = 5

def save_rgb(image, filePath, comp = OexrCompression.ZIP):
    '''Saves the rgb (image) in OpenEXR format. Expects a 3-chan uint32 image.'''

    if len(image.shape) != 3:
        raise Exception("Incorrect dimensions!")

    h, w, c = image.shape # expects Numpy convention (row, col) -> (height, width)

    if c != 3:
        raise Exception("Incorrect number of channels!")

    if image.dtype != "uint32":
        raise Exception("Incorrect type!, expected uint32")

    try:

        header = oe.Header(w, h)
        header["channels"] = {"R" : im.Channel(im.PixelType(oe.UINT)),
                              "G" : im.Channel(im.PixelType(oe.UINT)),
                              "B" : im.Channel(im.PixelType(oe.UINT))}

        header['compression'] = im.Compression(comp)

        of = oe.OutputFile(filePath, header)
        r_data = image[:, :, 0].tostring()
        g_data = image[:, :, 1].tostring()
        b_data = image[:, :, 2].tostring()
        of.writePixels({"R" : r_data, "G" : g_data, "B" : b_data})
        of.close()

    except:
        raise


def save_depth(image, filePath, comp = OexrCompression.ZIP):
    '''Saves the zBuffer (image) in OpenEXR format. Expects a 1-chann float32 image.'''

    if len(image.shape) != 2:
        raise Exception("Incorrect dimensions!")

    if image.dtype != "float32":
        raise Exception("Incorrect type!, expected float32")

    try:
        h, w = image.shape # expects Numpy convention (row, col) -> (height, width)
        header = oe.Header(w, h)
        header["channels"] = {"Z" : im.Channel(im.PixelType(oe.FLOAT))}

        header['compression'] = im.Compression(comp)

        of = oe.OutputFile(filePath, header)
        image_data = image.tostring()
        of.writePixels({"Z" : image_data})
        of.close()

    except:
        raise


def load_rgb(filePath):
    ''' Loads an rgb OpenEXR image.'''

    if oe.isOpenExrFile(filePath) is not True:
        raise Exception("File ", filePath, " does not exist!")

    try:
        ifi = oe.InputFile(filePath)

        # Compute size
        header = ifi.header()
        dw = header["dataWindow"]
        w, h = dw.max.x - dw.min.x + 1, dw.max.y - dw.min.y + 1

        # Read the three channels
        ifiType = header["channels"]["R"].type.v
        if ifiType is not im.PixelType.UINT:
            raise Exception("Only uint32 supported! (file is type ", ifiType)

        R = ifi.channel("R", im.PixelType(ifiType))
        G = ifi.channel("G", im.PixelType(ifiType))
        B = ifi.channel("B", im.PixelType(ifiType))
        ifi.close()

        image = np.zeros((h, w, 3), dtype = np.uint32) # order = "C"
        image[:, :, 0] = np.core.multiarray.fromstring(R, dtype = np.uint32).reshape(h, w)
        image[:, :, 1] = np.core.multiarray.fromstring(G, dtype = np.uint32).reshape(h, w)
        image[:, :, 2] = np.core.multiarray.fromstring(B, dtype = np.uint32).reshape(h, w)

    except:
        raise

    return image


def load_depth(filePath):
    ''' Loads an depth OpenEXR image.'''

    if oe.isOpenExrFile(filePath) is not True:
        raise Exception("File ", filePath, " does not exist!")

    try:
        ifi = oe.InputFile(filePath)

        # Compute size
        header = ifi.header()
        dw = header["dataWindow"]
        w, h = dw.max.x - dw.min.x + 1, dw.max.y - dw.min.y + 1

        # Read the three channels
        ifiType = header["channels"]["Z"].type.v
        if ifiType is not im.PixelType.FLOAT:
            raise Exception("Only float32 supported! (file is type ", ifiType)

        Z = ifi.channel("Z", im.PixelType(ifiType))
        ifi.close()

        image = np.zeros((h, w), dtype = np.float32) # order = "C"
        image[:, :] = np.core.multiarray.fromstring(Z, dtype = np.float32).reshape(h, w)

    except:
        raise

    return image
