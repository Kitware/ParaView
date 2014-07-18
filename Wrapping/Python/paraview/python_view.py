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


def numpy_to_image(numpy_array):
  """
  @brief Convert a numpy 2D or 3D array to a vtkImageData object
  @param numpy_array 2D or 3D numpy array containing image data
  @return vtkImageData with the numpy_array content
  """
  try:
    import numpy
  except:
    paraview.print_error("Error: Cannot import numpy")

  shape = numpy_array.shape
  if len(shape) < 2:
    raise Exception('numpy array must have dimensionality of at least 2')

  h, w = shape[0], shape[1]
  c = 1
  if len(shape) == 3:
    c = shape[2]

  # Reshape 2D image to 1D array suitable for conversion to a
  # vtkArray with numpy_support.numpy_to_vtk()
  linear_array = numpy.reshape(numpy_array, (w*h, c))

  try:
    from vtk.util import numpy_support
  except:
    paraview.print_error("Error: Cannot import vtk.util.numpy_support")

  vtk_array = numpy_support.numpy_to_vtk(linear_array)

  image = vtk.vtkImageData()
  image.SetDimensions(w, h, 1)
  image.AllocateScalars(vtk_array.GetDataType(), 4)
  image.GetPointData().GetScalars().DeepCopy(vtk_array)

  return image


def figure_to_data(figure):
  """
  @brief Convert a Matplotlib figure to a numpy 2D array with RGBA uint8 channels and return it.
  @param figure A matplotlib figure.
  @return A numpy 2D array of RGBA values.
  """
  # Draw the renderer
  try:
    import matplotlib
  except:
    paraview.print_error("Error: Cannot import matplotlib")

  figure.canvas.draw()

  # Get the RGBA buffer from the figure
  w, h = figure.canvas.get_width_height()

  try:
    import numpy
  except:
    paraview.print_error("Error: Cannot import numpy")

  buf = numpy.fromstring(figure.canvas.tostring_argb(), dtype=numpy.uint8)
  buf.shape = (h, w, 4)

  # canvas.tostring_argb gives pixmap in ARGB mode. Roll the alpha channel to have it in RGBA mode
  buf = numpy.roll(buf, 3, axis=2)

  return buf


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


def matplotlib_figure(width, height):
  """
  @brief Create a Matplotlib figure with specified width and height for rendering
  @param w Width of desired plot
  @param h Height of desired plot
  @return A Matplotlib figure
  """
  try:
    from matplotlib.backends.backend_agg import FigureCanvasAgg
  except:
    paraview.print_error("Error: Cannot import matplotlib.backends.backend_agg.FigureCanvasAgg")
  try:
    from matplotlib.figure import Figure
  except:
    paraview.print_error("Error: Cannot import matplotlib.figure.Figure")

  figure = Figure()
  figureCanvas = FigureCanvasAgg(figure)
  figure.set_dpi(72)
  figure.set_size_inches(float(width)/72.0, float(height)/72.0)

  return figure


def call_setup_data(setup_data_function, view):
  """
  @brief Utility function to call the user-defined setup_data function. This is
         meant to be called by the C++ side of the vtkPythonView class.
  @parameter view vtkPythonView object
  """
  if setup_data_function == None:
    return

  setup_data_function(view)


def call_render(render_function, view, width, height):
  """
  @brief Utility function to call the user-defined render function. This is
         called by the C++ side of the vtkPythonView class.
  @parameter view vtkPythonView object
  @parameter width Width of view
  @parameter height Height of view
  """
  if render_function == None:
    return

  # Check how many parameters it takes.
  num_args = render_function.__code__.co_argcount

  image = None
  if (num_args == 3):
    # Current-style render() function
    image = render_function(view, width, height)

  elif (num_args == 2):
    # Old-style render() function introduced in ParaView 4.1
    figure = matplotlib_figure(width, height)
    render_function(view, figure)

    image = figure_to_image(figure)

  view.SetImageData(image)
