#==============================================================================
#
#  Program:   ParaView
#  Module:    annotation.py
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
This module is used by vtkPythonAnnotationFilter.
"""
import paraview
from vtk.numpy_interface import dataset_adapter
from vtk.numpy_interface.algorithms import *

def _make_name_valid(name):
    return paraview.make_name_valid(name)

def ComputeAnnotation(self, inputDS, expression, t_value = 0, t_steps = [0,1], t_range = [0,1]):
    # init input object
    input = dataset_adapter.WrapDataObject(inputDS)

    # Add Fields names inside current namespace
    numberOfFields = input.GetFieldData().GetNumberOfArrays()
    for index in xrange(numberOfFields):
       fieldName = input.GetFieldData().GetAbstractArray(index).GetName()
       exec("%s = input.FieldData['%s']" % (_make_name_valid(fieldName), fieldName))

    # handle multi-block
    inputMB = []
    try:
        for block in input:
            inputMB.append(block)
    except:
        pass

    # Add time informations in current namespace
    t_index = 0
    try:
       t_index = t_steps.index(t_value)
    except:
       pass
    # Add extra naming
    time_value = t_value
    time_steps = t_steps
    time_range = t_range
    time_index = t_index

    # Evaluate expression
    exec("outputText = str(" + expression + ")")
    self.SetAnnotationValue(outputText)
