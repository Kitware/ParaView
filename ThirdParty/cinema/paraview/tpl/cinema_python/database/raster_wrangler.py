"""
Module that uses one of the available back end libraries to write out image
files for cinema's file store class.
"""

import numpy
import zlib
import os
import warnings

exrEnabled = False
try:
    import oexr_helper as exr
    exrEnabled = True
    print(
        "Imported OpenEXR, will default to *.exr for float images (z-buffer, "
        "value, etc.).")
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
        from paraview.vtk.vtkIOAsynchronous import vtkThreadedImageWriter
        from paraview.vtk.vtkCommonDataModel import vtkImageData
        from paraview import numpy_support as n2v
    else:
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
        from vtk.vtkIOAsynchronous import vtkThreadedImageWriter
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
        self.threadedwriter = None
        self.backends = set()
        if exrEnabled:
            self.backends.add("OpenEXR")
        elif vtkEnabled:
            self.backends.add("VTK")
            self.threadedwriter = vtkThreadedImageWriter()
            self.threadedwriter.Initialize()
        elif pilEnabled:
            self.backends.add("PIL")

        # self.dontCompressFloatVals = False  # don't expect we'll need this
        # self.dontConvertValsToFloat = False  # nor this

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

    def _make_writer(self, filename):
        "Internal function."
        extension = None
        parts = filename.split('.')
        if len(parts) > 1:
            extension = parts[-1]
        else:
            raise RuntimeError(
                "Filename has no extension, cannot guess writer to use")

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
            return vtkXMLImageDataWriter()  # noqa: F821
        else:
            raise RuntimeError(
                "Cannot infer filetype from extension:", extension)

    def _make_reader(self, filename):
        "Internal function."
        extension = None
        parts = filename.split('.')
        if len(parts) > 1:
            extension = parts[-1]
        else:
            raise RuntimeError(
                "Filename has no extension, please guess reader to use")

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
            return vtkXMLImageDataReader()  # noqa: F821
        else:
            raise RuntimeError(
                "Cannot infer filetype from extension:", extension)

    def genericreader(self, fname):
        """read generic binary data dump"""
        with open(fname, "r") as file:
            return file.read()

    def genericwriter(self, imageslice, fname):
        """write generic binary data dump"""
        if self.threadedwriter is not None:
            height = imageslice.shape[1]
            width = imageslice.shape[0]
            contig = imageslice.reshape(height*width)
            vtkarray = n2v.numpy_to_vtk(contig)
            id = vtkImageData()
            id.SetExtent(0, height-1, 0, width-1, 0, 0)
            id.GetPointData().SetScalars(vtkarray)
            self.threadedwriter.EncodeAndWrite(id, fname)
        else:
            with open(fname, "w") as file:
                file.write(imageslice)

    def rgbreader(self, fname):
        """opens a color image file and returns it as a color buffer"""
        if "VTK" in self.backends:
            reader = self._make_reader(fname)
            reader.SetFileName(fname)
            reader.Update()
            id = reader.GetOutput()
            ext = id.GetExtent()
            width = ext[1]+1
            height = ext[3]+1
            imageslice = n2v.vtk_to_numpy(id.GetPointData().GetArray(0))
            imageslice = imageslice.reshape(height, width, 3)
            return numpy.flipud(imageslice)
        elif "PIL" in self.backends:
            try:
                im = PIL.Image.open(fname)
                # this image could be any format, but we require RGB
                # as we handle this image
                conv_im = im.convert("RGB")

                return numpy.array(
                    conv_im, numpy.uint8).reshape(im.size[1], im.size[0], 3)
            except:
                return None

        else:
            print("Warning: need PIL or VTK to read from " + fname)

    def rgbwriter(self, imageslice, fname):
        """takes in a color buffer and writes it as an image file"""
        if "VTK" in self.backends:
            height = imageslice.shape[1]
            width = imageslice.shape[0]
            contig = imageslice.reshape(height*width, 3)
            vtkarray = n2v.numpy_to_vtk(contig)
            id = vtkImageData()
            id.SetExtent(0, height-1, 0, width-1, 0, 0)
            id.GetPointData().SetScalars(vtkarray)

            if self.threadedwriter is not None:
                self.threadedwriter.EncodeAndWrite(id, fname)
            else:
                writer = self._make_writer(fname)
                writer.SetInputData(id)
                writer.SetFileName(fname)
                writer.Write()

        elif "PIL" in self.backends:
            imageslice = numpy.flipud(imageslice)
            pimg = PIL.Image.fromarray(imageslice)
            pimg.save(fname)

        else:
            print("Warning: need PIL or VTK to write to " + fname)

    def valuewriter(self, imageSlice, fname, vrange):
        """ Takes in either a (1C) float or a RGB (3C) buffer and writes it as
        an image file."""

        # Adjust the filename, replace png with .im
        baseName, ext = os.path.splitext(fname)
        adjustedName = baseName + self.floatExtension()

        dimensions = imageSlice.shape
        if len(dimensions) == 2 and imageSlice.dtype == numpy.float32:
            # Given as single channel floating point buffer.
            self.zwriter(imageSlice, adjustedName)

        elif (len(dimensions) > 2) and (dimensions[2] == 3):
            # if self.dontConvertValsToFloat
            #     self.rgbwriter(imageSlice, fname)
            #     return

            # Given an RGB buffer
            # Convert it to a floating point buffer for consistency
            # TODO: just one copy of this code in cinema
            w0 = numpy.left_shift(imageSlice[:, :, 0].astype(numpy.uint32), 16)
            w1 = numpy.left_shift(imageSlice[:, :, 1].astype(numpy.uint32), 8)
            w2 = imageSlice[:, :, 2].astype(numpy.uint32)
            value = numpy.bitwise_or(w0, w1)
            value = numpy.bitwise_or(value, w2)
            value = numpy.subtract(value, 1)
            value = value.astype(numpy.float32)
            adjusted_val = numpy.divide(value, float(0xFFFFFE))

            self.zwriter(adjusted_val, adjustedName)

        else:
            raise ValueError("Invalid dimensions for a value raster.")

    def valuereader(self, fname, shape=None):
        """ Opens a value image file and returns it as either a color buffer
        or a floating point array (depending on how the image was exported)."""
        baseName, ext = os.path.splitext(fname)
        if ext == self.floatExtension():
            # Treat as single channel floating point buffer.
            return self.zreader(fname, shape)
        else:
            # Treat as a RGB buffer
            return self.rgbreader(fname)

    def floatExtension(self):
        """determine file extension for depth images"""
        if "OpenEXR" in self.backends:
            return ".exr"

        else:
            return ".im"

    def zreader(self, fname, shape=None):
        """reads a depth file to make a depth buffer"""

        if "OpenEXR" in self.backends:
            return exr.load_depth(fname)

        if "PIL" in self.backends:
            # for backwards compatibility with Bacall, remove a.s.a.p
            try:
                im = PIL.Image.open(fname)
                return numpy.array(
                    im, numpy.float32).reshape(im.size[1], im.size[0])
            except:
                pass

        # Try to open as zlib compressed files first
        baseName, ext = os.path.splitext(fname)
        try:
            # Adjust the filename, replace .im with .Z
            adjustedName = baseName + ".Z"

            with open(adjustedName, mode='rb') as file:
                compresseddata = file.read()
            flatarr = numpy.fromstring(zlib.decompress(compresseddata),
                                       numpy.float32)

            if not shape:
                shape = flatarr.shape

            imageslice = flatarr.reshape(shape)
        # Fall back on numpy compressed format
        except IOError:
            # Adjust the filename, replace .im with .npz
            adjustedName = baseName + ".npz"

            with open(adjustedName, mode='rb') as file:
                with numpy.load(file) as tz:
                    imageslice = tz[tz.files[0]]
        return imageslice

    def zwriter(self, imageslice, fname):
        """takes in a depth buffer and writes it as a depth file"""

        if "OpenEXR" in self.backends:
            imageslice = numpy.flipud(imageslice)
            exr.save_depth(imageslice, fname)
            return

        # if self.dontCompressFloatVals:
        #     if "VTK" in self.backends:
        #         height = imageslice.shape[1]
        #         width = imageslice.shape[0]
        #
        #         file = open(fname, mode='w')
        #         file.write("Image type: L 32F image\r\n")
        #         file.write("Name: A cinema depth image\r\n")
        #         file.write(
        #             "Image size (x*y): "+str(height) +
        #             "*" + str(width) + "\r\n")
        #         file.write("File size (no of images): 1\r\n")
        #         file.write(chr(26))
        #         imageslice.tofile(file)
        #         file.close()
        #         return
        #
        #     imageslice = numpy.flipud(imageslice)
        #     pimg = PIL.Image.fromarray(imageslice)
        #     # TODO:
        #     # don't let ImImagePlugin.py insert the Name: filename in
        #     line two. why? because ImImagePlugin.py reader has a 100
        #     character limit
        #     pimg.save(fname)

        imageslice = numpy.flipud(imageslice)
        # Adjust the filename, replace .im with .npz
        baseName, ext = os.path.splitext(fname)
        adjustedName = baseName + ".Z"

        if self.threadedwriter is not None:
            height = imageslice.shape[1]
            width = imageslice.shape[0]
            contig = imageslice.reshape(height*width)
            vtkarray = n2v.numpy_to_vtk(contig)
            id = vtkImageData()
            id.SetExtent(0, height-1, 0, width-1, 0, 0)
            id.GetPointData().SetScalars(vtkarray)
            self.threadedwriter.EncodeAndWrite(id, adjustedName)
        else:
            with open(adjustedName, mode='wb') as file:
                file.write(zlib.compress(numpy.array(imageslice)))

    def assertvalidimage(self, filename):
        """tests that a given file is syntactically correct"""

        # Try .Z form
        baseName, ext = os.path.splitext(filename)
        adjustedName = baseName + ".Z"
        if os.path.isfile(adjustedName):
            with open(adjustedName, mode='rb') as file:
                compresseddata = file.read()

            imageslice = numpy.fromstring(zlib.decompress(compresseddata),
                                          numpy.float32)
            if not isinstance(imageslice, numpy.ndarray):
                raise IOError(adjustedName + " does not load correctly.")
            return

        # Try .npz form
        adjustedName = baseName + ".npz"
        if os.path.isfile(adjustedName):
            with open(adjustedName, mode='rb') as file:
                with numpy.load(file) as tz:
                    if not isinstance(tz[tz.files[0]], numpy.ndarray):
                        raise IOError(
                            adjustedName + " does not load correctly.")
            return

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
