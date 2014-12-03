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

def _get_ns(self, do, association):
    if association == vtkDataObject.FIELD:
        # For FieldData, it gets tricky. In general, one would think we are going
        # to look at field data in inputDO directly -- same for composite datasets.
        # However, ExodusIIReader likes to put field data on leaf nodes insead.
        # So we also check leaf nodes, if the FieldData on the root is empty.

        # We explicitly call dsa.DataObject.GetFieldData to ensure that
        # when dealing with composite datasets, we get the FieldData on the
        # vtkCompositeDataSet itself, not in the leaf nodes.
        fieldData = dsa.DataObject.GetFieldData(do)
        if len(fieldData.keys()) == 0:
            # if this is a composite dataset, use field data from the first block with some
            # field data.
            if isinstance(do, dsa.CompositeDataSet):
                for dataset in do:
                    fieldData = dataset.GetFieldData()
                    if (not fieldData is None) and (len(fieldData.keys()) > 0): break
    else:
        fieldData = do.GetAttributes(association)
    arrays = calculator.get_arrays(fieldData)

    ns = {}
    ns["input"] = do
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
    return ns


def execute(self):
    """Called by vtkPythonAnnotationFilter."""
    expression = self.GetExpression()
    inputDO = self.GetCurrentInputDataObject()
    if not expression or not inputDO:
        return True

    inputs = [dsa.WrapDataObject(inputDO)]
    association = self.GetArrayAssociation()
    ns = _get_ns(self, inputs[0], association)

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


def execute_on_global_data(self):
    """Called by vtkAnnotateGlobalDataFilter."""
    inputDO = self.GetCurrentInputDataObject()
    if not inputDO:
        return True

    inputs = [dsa.WrapDataObject(inputDO)]
    association = self.GetArrayAssociation()
    ns = _get_ns(self, inputs[0], association)
    if not ns.has_key(self.GetFieldArrayName()):
        print >> stderr, "Failed to locate global array '%s'." % self.GetFieldArrayName()
        raise RuntimeError, "Failed to locate global array"

    array = ns[self.GetFieldArrayName()]
    chosen_element = array
    try:
        if self.GetNumberOfTimeSteps() > 0 and \
            array.shape[0] == self.GetNumberOfTimeSteps():
                chosen_element = array[ns["time_index"]]
    except AttributeError: pass

    try:
        # hack for string array.
        if chosen_element.IsA("vtkStringArray"):
            chosen_element = chosen_element.GetValue(0)
    except: pass
    expression = self.GetPrefix() if self.GetPrefix() else ""
    expression += str(chosen_element)
    self.SetComputedAnnotationValue(expression)
    return True

def execute_on_attribute_data(self, evaluate_locally):
    """Called by vtkAnnotateAttributeDataFilter."""
    inputDO = self.GetCurrentInputDataObject()
    if not inputDO:
        return True

    inputs = [dsa.WrapDataObject(inputDO)]
    association = self.GetArrayAssociation()
    ns = _get_ns(self, inputs[0], association)
    if not ns.has_key(self.GetArrayName()):
        print >> stderr, "Failed to locate array '%s'." % self.GetArrayName()
        raise RuntimeError, "Failed to locate array"

    if not evaluate_locally:
        return True

    array = ns[self.GetArrayName()]
    chosen_element = array[self.GetElementId()]
    expression = self.GetPrefix() if self.GetPrefix() else ""
    expression += str(chosen_element)
    self.SetComputedAnnotationValue(expression)
    return True
