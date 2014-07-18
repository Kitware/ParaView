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
try:
    import numpy as np
except ImportError:
    raise RuntimeError, "'numpy' module is not found. numpy is needed for "\
        "this functionality to work. Please install numpy and try again."

from paraview import calculator
from vtk import vtkDataObject
from vtk.numpy_interface import dataset_adapter as dsa


def execute(self):
    expression = self.GetExpression()
    inputDO = self.GetCurrentInputDataObject()
    if not expression or not inputDO:
        return True

    inputs = [dsa.WrapDataObject(inputDO)]
    association = self.GetArrayAssociation()
    if association == vtkDataObject.FIELD:
        # For FieldData, it gets tricky. In general, one would think we are going
        # to look at field data in inputDO directly -- same for composite datasets.
        # However, ExodusIIReader likes to put field data on leaf nodes insead.
        # So we also check leaf nodes, if the FieldData on the root is empty.

        # We explicitly call dsa.DataObject.GetFieldData to ensure that
        # when dealing with composite datasets, we get the FieldData on the
        # vtkCompositeDataSet itself, not in the leaf nodes.
        fieldData = dsa.DataObject.GetFieldData(inputs[0])
        if len(fieldData.keys()) == 0:
            # if this is a composite dataset, use field data from the first block with some
            # field data.
            if isinstance(inputs[0], dsa.CompositeDataSet):
                for dataset in inputs[0]:
                    fieldData = dataset.GetFieldData()
                    if (not fieldData is None) and (len(fieldData.keys()) > 0): break
    else:
        fieldData = inputs[0].GetAttributes(association)
    arrays = calculator.get_arrays(fieldData)

    ns = {}
    ns["input"] = inputs[0]
    if self.GetDataTimeValid():
        ns["time_value"] = self.GetDataTime()
        ns["t_value"] = ns["time_value"]

    if self.GetNumberOfTimeSteps() > 0:
        ns["time_steps"] = [self.GetTimeStep(x) for x in xrange(self.GetNumberOfTimeSteps())]
        ns["t_steps"] = ns["time_steps"]

    if self.GetTimeRangeValid():
        ns["time_range"] = self.GetTimeRange()
        ns["t_range"] = ns["time_range"]

    if self.GetDataTimeValid() and self.GetNumberOfTimeSteps() > 0:
        try:
            ns["time_index"] = ns["time_steps"].index(ns["time_value"])
            ns["t_index"] = ns["time_index"]
        except ValueError: pass
    ns.update(arrays)

    try:
        result = calculator.compute(inputs, expression, ns=ns)
    except:
        from sys import stderr
        print >> stderr, "Failed to evaluate expression '%s'. "\
            "The following exception stack should provide additional "\
            "developer specific information. This typically implies a malformed "\
            "expression. Verify that the expression is valid.\n\n" \
            "Variables in current scope are %s \n" % (expression, ns.keys())
        raise
    self.SetComputedAnnotationValue("%s" % result)
    return True
