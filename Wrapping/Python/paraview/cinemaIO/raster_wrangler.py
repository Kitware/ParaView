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
    Module that uses one of the available back end libraries to write out image
    files for cinema's file store class.
"""

import numpy

exrEnabled = False
try:
    import OexrHelper as exr
    exrEnabled = True
    print "Imported OpenEXR, will default to *.exr in z-buffer images."
except ImportError:
    pass

pilEnabled = False
try:
    import PIL.Image
    import PIL.ImImagePlugin
    pilEnabled = True
except ImportError:
    pass

vtkEnabled = False
try:
    import sys
    if "paraview" in sys.modules:
        import paraview.vtk
        import paraview.vtk.vtkIOImage
        from paraview.vtk.vtkIOImage import (vtkPNGWriter,
                                             vtkBMPWriter,
                                             vtkPNMWriter,
                                             vtkTIFFWriter,
                                             vtkJPEGWriter)
        from paraview.vtk.vtkCommonDataModel import vtkImageData
        from paraview import numpy_support as n2v
    else:
        import vtk
        from vtk import (vtkPNGWriter,
                         vtkBMPWriter,
                         vtkPNMWriter,
                         vtkTIFFWriter,
                         vtkJPEGWriter,
                         vtkImageData)
        from vtk.util import numpy_support as n2v
    vtkEnabled = True
except ImportError:
    pass

def _make_writer(filename):
    "Internal function."
    extension = None
    parts = filename.split('.')
    if len(parts) > 1:
        extension = parts[-1]
    else:
        raise RuntimeError, "Filename has no extension, please specify a write"

    if extension == 'png':
        return vtkPNGWriter()
    elif extension == 'bmp':
        return vtkBMPWriter()
    elif extension == 'ppm':
        return vtkPNMWriter()
    elif extension == 'tif' or extension == 'tiff':
        return vtkTIFFWriter()
    elif extension == 'jpg' or extension == 'jpeg':
        return vtkJPEGWriter()
    else:
        raise RuntimeError, "Cannot infer filetype from extension:", extension

def rgbwriter(imageslice, fname):
    if pilEnabled:
        imageslice = numpy.flipud(imageslice)
        pimg = PIL.Image.fromarray(imageslice)
        pimg.save(fname)
        return

    if vtkEnabled:
        height = imageslice.shape[1]
        width = imageslice.shape[0]
        contig = imageslice.reshape(height*width,3)
        vtkarray = n2v.numpy_to_vtk(contig)
        id = vtkImageData()
        id.SetExtent(0, height-1, 0, width-1, 0, 0)
        id.GetPointData().SetScalars(vtkarray)

        writer = _make_writer(fname)
        writer.SetInputData(id)
        writer.SetFileName(fname)
        writer.Write()
        return

    print "Warning: need PIL or VTK to write to " + fname

def zwriter(imageslice, fname):
    if exrEnabled:
        imageslice = numpy.flipud(imageslice)
        exr.save_depth(imageslice, fname)
        return

    if pilEnabled:
        imageslice = numpy.flipud(imageslice)
        pimg = PIL.Image.fromarray(imageslice)
        #TODO:
        # don't let ImImagePlugin.py insert the Name: filename in line two
        # why? because ImImagePlugin.py reader has a 100 character limit
        pimg.save(fname)
        return

    if vtkEnabled:
        height = imageslice.shape[1]
        width = imageslice.shape[0]

        file = open(fname, mode='w')
        file.write("Image type: L 32F image\r\n")
        file.write("Name: A cinema depth image\r\n")
        file.write("Image size (x*y): "+str(height) + "*" + str(width) + "\r\n")
        file.write("File size (no of images): 1\r\n")
        file.write(chr(26))
        imageslice.tofile(file)
        file.close()
        return

    print "Warning: need OpenEXR or PIL or VTK to write to " + fname
