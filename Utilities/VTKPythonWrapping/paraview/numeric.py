#==============================================================================
#
#  Program:   ParaView
#  Module:    numeric.py
#
#  Copyright (c) Kitware, Inc.
#  All rights reserved.
#  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
#
#     This software is distributed WITHOUT ANY WARRANTY; without even
#     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#     PURPOSE.  See the above copyright notice for more information.
#
#==============================================================================
r"""
This module provides functions to vtk data arrays to NumPy arrays.
"""

__num_py_available__ = False
try:
    import Numeric
    __num_py_available__ = True
except:
    raise """NumPy module "Numeric" is not accessible. Please make sure
           that NumPy is installed correctly."""

# These types are returned by GetDataType to indicate pixel type.
VTK_VOID            = 0
VTK_BIT             = 1
VTK_CHAR            = 2
VTK_UNSIGNED_CHAR   = 3
VTK_SHORT           = 4
VTK_UNSIGNED_SHORT  = 5
VTK_INT             = 6
VTK_UNSIGNED_INT    = 7
VTK_LONG            = 8
VTK_UNSIGNED_LONG   = 9
VTK_FLOAT           =10
VTK_DOUBLE          =11
VTK_ID_TYPE         =12

__typeDict = { VTK_CHAR:Numeric.Int8,
                   VTK_UNSIGNED_CHAR:Numeric.UnsignedInt8,
                   VTK_SHORT:Numeric.Int16,
                   VTK_UNSIGNED_SHORT:Numeric.Int16,
                   VTK_INT:Numeric.Int32,
                   VTK_FLOAT:Numeric.Float32,
                   VTK_DOUBLE:Numeric.Float64 }

def getarray(vtkarray):
    """This function takes a vtkDataArray of any type and converts it to a
     NumPy array of appropriate type and dimensions."""
    global __typeDict__
    global __num_py_available__
    if not __num_py_available__:
        raise "NumPy module is not available."
    #create a numpy array of the correct type.
    vtktype = vtkarray.GetDataType()
    if not __typeDict__.has_key(vtktype):
        raise "Cannot convert data arrays of the type %s" \
          % vtkarray.GetDataTypeAsString()
    type = __typeDict__[vtktype]
    num_tuples = vtkarray.GetNumberOfTuples()
    num_comps = vtkarray.GetNumberOfComponents()
    size = num_comps * num_tuples
    imArray = Numeric.empty((size,), type)
    vtkarray.ExportToVoidPointer(imArray)
    # re-shape the array to current number of rows and columns.
    imArray = Numeric.reshape(imArray, (num_tuples, num_comps))
    return imArray

