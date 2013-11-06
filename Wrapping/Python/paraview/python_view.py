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
