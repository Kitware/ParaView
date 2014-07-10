r"""python_view is a module providing access to a PythonView. It is
possible to use the PythonView API directly, but this module provides
convenience classes in Python.
"""
#==============================================================================
#
#  Program:   ParaView
#  Module:    python_view.py
#
#  Copyright (c) Kitware, Inc.
#  All rights reserved.
#  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
#
#     This software is distributed WITHOUT ANY WARRANTY without even
#     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#     PURPOSE.  See the above copyright notice for more information.
#
#==============================================================================
import paraview
import vtk

from vtkPVServerImplementationCorePython import *
from vtkPVClientServerCoreCorePython import *
from vtkPVServerManagerCorePython import *

try:
  from vtkPVServerManagerDefaultPython import *
except:
  paraview.print_error("Error: Cannot import vtkPVServerManagerDefaultPython")
try:
  from vtkPVServerManagerRenderingPython import *
except:
  paraview.print_error("Error: Cannot import vtkPVServerManagerRenderingPython")
try:
  from vtkPVServerManagerApplicationPython import *
except:
  paraview.print_error("Error: Cannot import vtkPVServerManagerApplicationPython")
from vtkPVCommonPython import *


def figure_to_data(figure):
  """
  @brief Convert a Matplotlib figure to a numpy 2D array with RGBA uint8 channels and return it.
  @param figure A matplotlib figure.
  @return A numpy 2D array of RGBA values.
  """
  # Draw the renderer
  import matplotlib
  figure.canvas.draw()

  # Get the RGBA buffer from the figure
  w, h = figure.canvas.get_width_height()

  import numpy
  buf = numpy.fromstring(figure.canvas.tostring_argb(), dtype=numpy.uint8)
  buf.shape = (h, w, 4)

  # canvas.tostring_argb gives pixmap in ARGB mode. Roll the alpha channel to have it in RGBA mode
  buf = numpy.roll(buf, 3, axis=2)

  return buf


def numpy_to_image(numpy_array):
  """
  @brief Convert a numpy 2D or 3D array to a vtkImageData object
  @param numpy_array 2D or 3D numpy array containing image data
  @return vtkImageData with the numpy_array content
  """
  shape = numpy_array.shape
  if len(shape) < 2:
    raise Exception('numpy array must have dimensionality of at least 2')

  h, w = shape[0], shape[1]
  c = 1
  if len(shape) == 3:
    c = shape[2]

  # Reshape 2D image to 1D array suitable for conversion to a
  # vtkArray with numpy_support.numpy_to_vtk()
  import numpy
  linear_array = numpy.reshape(numpy_array, (w*h, c))

  from vtk.util import numpy_support
  vtk_array = numpy_support.numpy_to_vtk(linear_array)

  image = vtk.vtkImageData()
  image.SetDimensions(w, h, 1);
  image.AllocateScalars(vtk_array.GetDataType(), 4);
  image.GetPointData().GetScalars().DeepCopy(vtk_array)

  return image


def figure_to_image(figure):
  """
  @brief Convert a Matplotlib figure to a vtkImageData with RGBA unsigned char channels
  @param figure A matplotlib figure.
  @return a vtkImageData with the Matplotlib figure content
  """
  buf = figure_to_data(figure)

  # Flip rows to be suitable for vtkImageData.
  buf = buf[::-1,:,:].copy()

  return numpy_to_image(buf)
