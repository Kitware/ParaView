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
from paraview import vtk
from paraview.vtk import dataset_adapter
from numpy import *
from paraview.vtk.algorithms import *
from paraview import servermanager
if servermanager.progressObserverTag:
    servermanager.ToggleProgressPrinting()

def __vtk_in1d(a, b):
    return array([item in b for item in a])

try:
    contains = in1d
except NameError:
    # older versions of numpy don't have in1d function.
    # in1d was introduced in numpy 1.4.0.
    contains = __vtk_in1d

def ComputeAnnotation(self, inputDS, expression, t_value = 0, t_steps = [0,1], t_range = [0,1]):
    # init input object
    input = dataset_adapter.WrapDataObject(inputDS)

    # Add Fields names inside current namespace
    numberOfFields = input.GetFieldData().GetNumberOfArrays()
    for index in xrange(numberOfFields):
       fieldName = input.GetFieldData().GetArray(index).GetName()
       exec("%s = input.FieldData['%s']" % (fieldName, fieldName))

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
