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
import os
import warnings

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
        from paraview.vtk.vtkIOImage import (vtkPNGReader,
                                             vtkBMPReader,
                                             vtkPNMReader,
                                             vtkTIFFReader,
                                             vtkJPEGReader,
                                             vtkPNGWriter,
                                             vtkBMPWriter,
                                             vtkPNMWriter,
                                             vtkTIFFWriter,
                                             vtkJPEGWriter)
        from paraview.vtk.vtkCommonDataModel import vtkImageData
        from paraview import numpy_support as n2v
    else:
        import vtk
        from vtk import (vtkPNGReader,
                         vtkBMPReader,
                         vtkPNMReader,
                         vtkTIFFReader,
                         vtkJPEGReader,
                         vtkPNGWriter,
                         vtkBMPWriter,
                         vtkPNMWriter,
                         vtkTIFFWriter,
                         vtkJPEGWriter,
                         vtkImageData)
        from vtk.util import numpy_support as n2v
    vtkEnabled = True
except ImportError:
    pass

class RasterWrangler(object):
    """
    Isolates the specifics of raster file formats from the cinema store.
    In particular this delegates the task to one or more subsidiary modules.
    The choice of which is open to facilitate packaging in different
    places, i.e. PIL for desktop and small packages, VTK for HPC contexts.
    """
    def __init__(self):
        self.backends = set()
        if exrEnabled:
            self.backends.add("OpenEXR")
        elif pilEnabled:
            self.backends.add("PIL")
        elif vtkEnabled:
            self.backends.add("VTK")

    def enableOpenEXR(self):
        """Try to turn on OpenEXR file IO support"""
        if exrEnabled:
            self.backends.add("OpenEXR")
        else:
            warnings.warn("OpenEXR module not found", ImportWarning)

    def enablePIL(self):
        """Try to turn on PIL file IO support"""
        if pilEnabled:
            self.backends.add("PIL")
        else:
            warnings.warn("PIL module not found", ImportWarning)

    def enableVTK(self):
        """Try to turn on VTK file IO support"""
        if vtkEnabled:
            self.backends.add("VTK")
        else:
            warnings.warn("VTK module not found", ImportWarning)

    def _make_writer(self,filename):
        "Internal function."
        extension = None
        parts = filename.split('.')
        if len(parts) > 1:
            extension = parts[-1]
        else:
            raise RuntimeError, "Filename has no extension, cannot guess writer to use"

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
        elif extension == 'vti':
            return vtkXMLImageDataWriter()
        else:
            raise RuntimeError, "Cannot infer filetype from extension:", extension

    def _make_reader(self,filename):
        "Internal function."
        extension = None
        parts = filename.split('.')
        if len(parts) > 1:
            extension = parts[-1]
        else:
            raise RuntimeError, "Filename has no extension, please guess reader to use"

        if extension == 'png':
            return vtkPNGReader()
        elif extension == 'bmp':
            return vtkBMPReader()
        elif extension == 'ppm':
            return vtkPNMReader()
        elif extension == 'tif' or extension == 'tiff':
            return vtkTIFFReader()
        elif extension == 'jpg' or extension == 'jpeg':
            return vtkJPEGReader()
        elif extension == 'vti':
            return vtkXMLImageDataReader()
        else:
            raise RuntimeError, "Cannot infer filetype from extension:", extension

    def genericreader(self, fname):
        """read generic binary data dump"""
        with open(fname, "r") as file:
            return file.read()

    def genericwriter(self, imageslice, fname):
        """write generic binary data dump"""
        with open(fname, "w") as file:
            file.write(imageslice)

    def rgbreader(self, fname):
        """opens a color image file and returns it as a color buffer"""
        if "VTK" in self.backends:
            height = imageslice.shape[1]
            width = imageslice.shape[0]
            contig = imageslice.reshape(height*width,3)
            vtkarray = n2v.numpy_to_vtk(contig)
            id = vtkImageData()
            id.SetExtent(0, height-1, 0, width-1, 0, 0)
            id.GetPointData().SetScalars(vtkarray)

            writer = self._make_writer(fname)
            writer.SetInputData(id)
            writer.SetFileName(fname)
            writer.Write()

        elif "PIL" in self.backends:
            try:
                im = PIL.Image.open(fname)
                #print "read", fname
                return numpy.array(im, numpy.uint8).reshape(im.size[1],im.size[0],3)
            except:
                #print "no such file", fname
                return None

        else:
            print "Warning: need PIL or VTK to read from " + fname

    def rgbwriter(self, imageslice, fname):
        """takes in a color buffer and writes it as an image file"""
        if "VTK" in self.backends:
            height = imageslice.shape[1]
            width = imageslice.shape[0]
            contig = imageslice.reshape(height*width,3)
            vtkarray = n2v.numpy_to_vtk(contig)
            id = vtkImageData()
            id.SetExtent(0, height-1, 0, width-1, 0, 0)
            id.GetPointData().SetScalars(vtkarray)

            writer = self._make_writer(fname)
            writer.SetInputData(id)
            writer.SetFileName(fname)
            writer.Write()

        elif "PIL" in self.backends:
            imageslice = numpy.flipud(imageslice)
            pimg = PIL.Image.fromarray(imageslice)
            pimg.save(fname)

        else:
            print "Warning: need PIL or VTK to write to " + fname

    def zfileextension(self):
        """determine file extension for depth images"""
        if "OpenEXR" in self.backends:
            return ".exr"

        else:
            return ".im"

    def zreader(self, fname):
        """reads a depth file to make a depth buffer"""
        if "OpenEXR" in self.backends:
            return exr.load_depth(fname)

        elif "PIL" in self.backends:
            try:
                im = PIL.Image.open(fname)
                #print "read", fname
                return numpy.array(im, numpy.float32).reshape(im.size[1],im.size[0])
            except:
                #print "no such file", fname
                return None
        else:
            print "Warning: need OpenEXR or PIL to read from " + fname

    def zwriter(self, imageslice, fname):
        """takes in a depth buffer and writes it as a depth file"""
        if "OpenEXR" in self.backends:
            imageslice = numpy.flipud(imageslice)
            exr.save_depth(imageslice, fname)

        elif "VTK" in self.backends:
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

        elif "PIL" in self.backends:
            imageslice = numpy.flipud(imageslice)
            pimg = PIL.Image.fromarray(imageslice)
            #TODO:
            # don't let ImImagePlugin.py insert the Name: filename in line two
            # why? because ImImagePlugin.py reader has a 100 character limit
            pimg.save(fname)

        else:
            print "Warning: need OpenEXR or PIL or VTK to write to " + fname

    def assertvalidimage(self, filename):
        """tests that a given file is syntactically correct"""
        if not os.path.isfile(filename):
            raise IOError(filename + " does not exist.")

        if "OpenEXR" in self.backends:
            if not exr.isOpenExrFile(filename):
                raise IOError(filename + " cannot be opened using OpenEXR.")

        elif "VTK" in self.backends:
            reader = self._make_reader(filename)
            if not reader.CanReadFile(filename):
                raise IOError("VTK Cannot open file " + filename)

        elif "PIL" in self.backends:
            try:
                PIL.Image.open(filename)
            except IOError:
                raise

        else:
            raise RuntimeError(
                "Warning: need OpenEXR or PIL or VTK to validate file store")
