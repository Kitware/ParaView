"""selection is a module for defining and using selections via Python. It provides
a convenience layer to provide functionality provided by the underlying C++ classes.

A simple example::

  from paraview.selection import *

  # Create a new sphere proxy on the active connection and register it
  # in the sources group.
  sphere = Sphere(ThetaResolution=16, PhiResolution=32)

  # Show the sphere
  Show(sphere)

  # Select cells falling inside a rectangle defined by two points on a diagonal,
  # (300, 400) and (600, 800).
  SelectSurfaceCells(Rectangle=[300, 400, 600, 800])

  # Add cells on processor 0 with ids 1 and 2 to the current selection.
  SelectIDs(IDs=[0, 1, 0, 2], FieldType='CELL', Source=sphere, Modifier='ADD')

  # Remove cells on processor 0 with ids 4 and 5 from the current selection.
  SelectIDS(IDs=[0, 4, 0, 5], FieldType='CELL', Source=sphere, Modifier='SUBTRACT')

  # Extract the currently extracted cells
  es = ExtractSelection()
  Show(es)

"""
#==============================================================================
#
#  Program:   ParaView
#  Module:    selection.py
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

from __future__ import absolute_import, division, print_function

import paraview
from paraview import servermanager as sm
import paraview.simple

import sys

class SelectionProxy(sm.Proxy):
    """ Special proxy wrapper type for Selections.
    """
    def __init__(self, **args):
        super(SelectionProxy, self).__init__(**args)

def _createSelection(proxyname, **args):
    """ Utility function to create a selection source. Basically a factory function
    that creates a proxy of a given name and hands off keyword arguments to the newly
    created proxy.
    """
    session = sm.ActiveConnection.Session
    pxm = sm.ProxyManager(session)

    proxy = pxm.NewProxy('sources', proxyname)

    s = SelectionProxy(proxy=proxy)
    for name, value in args.items():
        # Warning: this will add only the attributes initially passed in when creating the proxy.
        s.add_attribute(name, value)
        s.SetPropertyWithName(name, value)

    return s

def _collectSelectionPorts(selectedReps, selectionSources, SelectBlocks=False, Modifier=None):
    """ Utility to collect all the selection output ports from a list of selected representations
    and sources. Selects blocks instead of cells if the SelectBlocks option is True.
    The Modifier can be used to modify the currently existing selection through one of the values
    'ADD', 'SUBTRACT', 'TOGGLE' or None if the new selection should replace the existing one.

    """
    outputPorts = []

    if not selectedReps or selectedReps.GetNumberOfItems() <= 0:
        return outputPorts

    if not selectionSources or selectionSources.GetNumberOfItems() <= 0:
        return outputPorts

    if selectedReps.GetNumberOfItems() != selectionSources.GetNumberOfItems():
        return outputPorts

    for i in range(0, selectedReps.GetNumberOfItems()):
        repr = selectedReps.GetItemAsObject(i)
        selectionSource = selectionSources.GetItemAsObject(i)

        if not repr:
            continue

        # Ensure selected representation is registered with the proxy manager
        pxm = repr.GetSessionProxyManager()
        if not pxm:
            return

        # Get the output port from the representation input
        inputProperty = repr.GetProperty("Input")
        selectedSource = inputProperty.GetProxy(0)
        portNumber = inputProperty.GetOutputPortForConnection(0)

        outputPort = selectedSource.GetOutputPort(portNumber)

        # Get the output port's proxy. That is the selected source.
        outputProxy = outputPort.GetSourceProxy()

        # Convert block selection from index-based selection
        from paraview.modules.vtkRemotingViews import vtkSMSelectionHelper
        import paraview.vtk as vtk
        if SelectBlocks:
            selectionSource = vtkSMSelectionHelper.ConvertSelection(
                vtk.vtkSelectionNode.BLOCKS, selectionSource, selectedSource, portNumber)

        # Handle selection modifier
        if Modifier == 'ADD':
            vtkSMSelectionHelper.MergeSelection(selectionSource, selectedSource.GetSelectionInput(portNumber),
              selectedSource, outputPort.GetPortIndex())
        elif Modifier == 'SUBTRACT':
            vtkSMSelectionHelper.SubtractSelection(selectionSource, selectedSource.GetSelectionInput(portNumber),
              selectedSource, outputPort.GetPortIndex())
        elif Modifier == 'TOGGLE':
            vtkSMSelectionHelper.ToggleSelection(selectionSource, selectedSource.GetSelectionInput(portNumber),
              selectedSource, outputPort.GetPortIndex())

        # Set the selection input on the selected port of the selected source proxy
        selectedSource.SetSelectionInput(portNumber, selectionSource, outputPort.GetPortIndex())

        if SelectBlocks:
            # A new selection proxy was allocated when converting to a block
            # selection, so we need to delete unregsiter it for garbage collection
            selectionSource.UnRegister(None)

        # Add output port to list of ports and return it
        outputPorts.append(outputPort)

    return outputPorts

def _surfaceSelectionHelper(rectOrPolygon, view, type, Modifier=None):
    """ Selects mesh elements on a surface

    - rectOrPolygon - represents either a rectangle or polygon. If a rectangle, consists of a list
      containing the bottom left (x1, y1) and top right (x2, y2) corner of a rectangle defining the
      selection region in the format [x1, y1, x2, y2]. If a polygon, list of points defining the
      polygon in x-y pairs (e.g., [x1, y1, x2, y2, x3, y3,...])
    - view - the view in which to make the selection
    - type - the type of mesh element: 'POINT', 'CELL', or 'BLOCK'
    - Modifier - 'ADD', 'SUBTRACT', 'TOGGLE', or None to define whether and how the selection
      should modify the existing selection.
    """
    if not view.SMProxy.IsA('vtkSMRenderViewProxy'):
        return

    # ensure the view has rendered so that the selection is valid
    paraview.simple.Render(view)

    if len(rectOrPolygon) == 0:
        rectOrPolygon = [0, 0, view.ViewSize[0], view.ViewSize[1]]

    from paraview.vtk import vtkCollection, vtkIntArray

    selectedReps = vtkCollection()
    selectionSources = vtkCollection()

    if type.startswith('POLYGON'):
        polygon = vtkIntArray()
        polygon.SetNumberOfComponents(2)

        for component in rectOrPolygon:
            polygon.InsertNextValue(component)

    if type == 'POINT':
        view.SelectSurfacePoints(rectOrPolygon, selectedReps, selectionSources, 0)
    elif type == 'CELL' or type == 'BLOCK':
        view.SelectSurfaceCells(rectOrPolygon, selectedReps, selectionSources, 0)
    elif type == 'FRUSTUM_POINTS':
        view.SelectFrustumPoints(rectOrPolygon, selectedReps, selectionSources, 0)
    elif type == 'FRUSTUM_CELLS':
        view.SelectFrustumCells(rectOrPolygon, selectedReps, selectionSources, 0)
    elif type == 'POLYGON_POINTS':
        view.SelectPolygonPoints(polygon, selectedReps, selectionSources, 0)
    elif type == 'POLYGON_CELLS':
        view.SelectPolygonCells(polygon, selectedReps, selectionSources, 0)
    else:
        raise RuntimeError("Invalid type %s" % type)

    _collectSelectionPorts(selectedReps, selectionSources, type == 'BLOCK', Modifier)

    paraview.simple.Render(view)

def SelectSurfacePoints(Rectangle=[], Polygon=[], View=None, Modifier=None):
    """Select visible points within a rectangular or polygon region.

    - Rectangle - list containing bottom left (x1, y1) and top right (x2, y2) corner of a
      rectangle defining the selection region in the format [x1, y1, x2, y2]. Defined in pixels.
      If not empty, the Polygon parameter must be empty or None.
    - Polygon - list of 2D points defining a polygon in which visible points should be selected,
       e.g., [x1, y1, x2, y2, ..., xn, yn]. Defined in pixels. If not empty, the Rectangle parameter
       must be empty or None.
    - View - the view in which to perform the selection. If None, uses the current active view.
    - Modifier - 'ADD', 'SUBTRACT', 'TOGGLE', or None to define whether and how the selection
      should modify the existing selection.
    """
    if not View:
        View = paraview.simple.GetActiveView()

    if Rectangle and Polygon:
        raise RuntimeError("Can only have one of Rectangle or Polygon named parameters set.")
    elif Rectangle:
        return _surfaceSelectionHelper(Rectangle, View, 'POINT', Modifier)
    elif Polygon:
        return _surfaceSelectionHelper(Polygon, View, 'POLYGON_POINTS', Modifier)
    else:
        raise RuntimeError("Need to set either the Rectangle or Polygon named parameters.")

    return None

def SelectSurfaceCells(Rectangle=[], Polygon=[], View=None, Modifier=None):
    """Select visible cells within a rectangular or polygon region.

    - Rectangle - list containing bottom left (x1, y1) and top right (x2, y2) corner of a
      rectangle defining the selection region in the format [x1, y1, x2, y2]. Defined in pixels.
      If not empty, the Polygon parameter must be empty or None.
    - Polygon - list of 2D points defining a polygon in which visible points should be selected,
       e.g., [x1, y1, x2, y2, ..., xn, yn]. Defined in pixels. If not empty, the Rectangle parameter
       must be empty or None.
    - View - the view in which to perform the selection. If None, uses the current active view.
    - Modifier - 'ADD', 'SUBTRACT', 'TOGGLE', or None to define whether and how the selection
      should modify the existing selection.
    """
    if not View:
        View = paraview.simple.GetActiveView()

    if Rectangle and Polygon:
        raise RuntimeError("Can only have one of Rectangle or Polygon named parameters set.")
    if Rectangle:
        return _surfaceSelectionHelper(Rectangle, View, 'CELL', Modifier)
    elif Polygon:
        return _surfaceSelectionHelper(Polygon, View, 'POLYGON_CELLS', Modifier)
    else:
        raise RuntimeError("")

def SelectSurfaceBlocks(Rectangle=[], View=None, Modifier=None):
    """Select visible blocks within a rectangular region.

    - Rectangle - list containing bottom left (x1, y1) and top right (x2, y2) corner of a
      rectangle defining the selection region in the format [x1, y1, x2, y2]. Defined in pixels.
    - View - the view in which to perform the selection. If None, uses the current active view.
    - Modifier - 'ADD', 'SUBTRACT', 'TOGGLE', or None to define whether and how the selection
      should modify the existing selection.
    """
    if not View:
        View = paraview.simple.GetActiveView()

    if not Modifier:
        # Clear the current selection - work around default behavior or utility function, which is
        # to add blocks.
        ClearSelection()

        # need to trigger render since otherwise the representation for the
        # selection that was just cleared may not have updated.
        paraview.simple.Render(View)

    return _surfaceSelectionHelper(Rectangle, View, 'BLOCK', Modifier)

def SelectPointsThrough(Rectangle=[], View=None):
    """Select all points within a rectangular region regardless of their visibility.

    - Rectangle - list containing bottom left (x1, y1) and top right (x2, y2) corner of a
      rectangle defining the selection region in the format [x1, y1, x2, y2]. Defined in pixels.
    - View - the view in which to perform the selection. If None, uses the current active view.
    """
    if not View:
        View = paraview.simple.GetActiveView()

    return _surfaceSelectionHelper(Rectangle, View, 'FRUSTUM_POINTS', None)

def SelectCellsThrough(Rectangle=[], View=None):
    """Select all cells within a rectangular region regardless of their visibility.

    - Rectangle - list containing bottom left (x1, y1) and top right (x2, y2) corner of a
      rectangle defining the selection region in the format [x1, y1, x2, y2]. Defined in pixels.
    - View - the view in which to perform the selection. If None, uses the current active view.
    """
    if not View:
        View = paraview.simple.GetActiveView()

    return _surfaceSelectionHelper(Rectangle, View, 'FRUSTUM_CELLS', None)

def _selectIDsHelper(proxyname, IDs=[], FieldType='POINT', ContainingCells=False, Source=None, Modifier=None):
    """Selects IDs of a given field type.

    - proxyname - name of the selection proxy to instantiate
    - IDs - list of IDs of attribute types to select.
    - FieldType - type of attribute to select, e.g., 'POINT', 'CELL'
    - ContainingCells - if True and FieldType is 'POINT', select the cells containing the
        points corresponding to the given IDs.
    - Source - If not None, specifies the sources whose elements should be selected by ID.
    - Modifier - 'ADD', 'SUBTRACT', 'TOGGLE', or None to define whether and how the selection
        should modify the existing selection.
    """
    if not Source:
        Source = paraview.simple.GetActiveSource()

    repr = paraview.simple.GetRepresentation(Source)

    import paraview.vtk as vtk
    reprCollection = vtk.vtkCollection()
    reprCollection.AddItem(repr.SMProxy)

    selection = _createSelection(proxyname, IDs=IDs, FieldType=FieldType, ContainingCells=ContainingCells)
    if selection:
        selectionCollection = vtk.vtkCollection()
        selectionCollection.AddItem(selection.SMProxy)

        _collectSelectionPorts(reprCollection, selectionCollection, Modifier=Modifier)

    Source.UpdateVTKObjects()

    paraview.simple.RenderAllViews()

def SelectGlobalIDs(IDs=[], FieldType='POINT', ContainingCells=False, Source=None, Modifier=None):
    """Select attributes by global IDs.

    - IDs - list of IDs of attribute types to select. Defined as a list of global IDs.
    - FieldType - type of attribute to select, e.g., 'POINT', 'CELL'
    - ContainingCells - if True and FieldType is 'POINT', select the cells containing the
        points corresponding to the given IDs.
    - Source - If not None, specifies the sources whose elements should be selected by ID.
    - Modifier - 'ADD', 'SUBTRACT', 'TOGGLE', or None to define whether and how the selection
        should modify the existing selection.
    """
    _selectIDsHelper('GlobalIDSelectionSource', **locals())

def SelectPedigreeIDs(IDs=[], FieldType='POINT', ContainingCells=False, Source=None, Modifier=None):
    """Select attributes by Pedigree IDs.

    - IDs - list of IDs of attribute types to select. Defined as (domain, ID) pairs
      interleaved in a single list.
    - FieldType - type of attribute to select, e.g., 'POINT', 'CELL'
    - ContainingCells - if True and FieldType is 'POINT', select the cells containing the
        points corresponding to the given IDs.
    - Source - If not None, specifies the sources whose elements should be selected by ID.
    - Modifier - 'ADD', 'SUBTRACT', 'TOGGLE', or None to define whether and how the selection
        should modify the existing selection.
    """
    _selectIDsHelper('PedigreeIDSelectionSource', **locals())

def SelectIDs(IDs=[], FieldType='POINT', ContainingCells=False, Source=None, Modifier=None):
    """Select attributes by attribute IDs.

    - IDs - list of IDs of attribute types to select. Defined as (process number, attribute ID) pairs
      interleaved in a single list. For multiblock datasets, this will select attributes on all
      blocks of the provided (processor number, attribute ID) pairs
    - FieldType - type of attribute to select, e.g., 'POINT', 'CELL'
    - ContainingCells - if True and FieldType is 'POINT', select the cells containing the
        points corresponding to the given IDs.
    - Source - If not None, specifies the sources whose elements should be selected by ID.
    - Modifier - 'ADD', 'SUBTRACT', 'TOGGLE', or None to define whether and how the selection
        should modify the existing selection.
    """
    _selectIDsHelper('IDSelectionSource', **locals())

def SelectCompositeDataIDs(IDs=[], FieldType='POINT', ContainingCells=False, Source=None, Modifier=None):
    """Select attributes by composite attribute IDs.

    - IDs - list of IDs of attribute types to select. Defined as 3-tuples of
      (flat block index, process number, attribute ID) interleaved in a single list.
    - FieldType - type of attribute to select, e.g., 'POINT', 'CELL'
    - ContainingCells - if True and FieldType is 'POINT', select the cells containing the
        points corresponding to the given IDs.
    - Source - If not None, specifies the sources whose elements should be selected by ID.
    - Modifier - 'ADD', 'SUBTRACT', 'TOGGLE', or None to define whether and how the selection
        should modify the existing selection.
    """
    selectIDsHelper('CompositeDataIDSelectionSource', **locals())

def SelectHierarchicalDataIDs(IDs=[], FieldType='POINT', ContainingCells=False, Source=None, Modifier=None):
    """Select attributes by hierarchical data IDs.

    - IDs - list of IDs of attribute types to select. Defined as 3-tuples of
      (level, index, attribute ID) interleaved in a single list.
    - FieldType - type of attribute to select, e.g., 'POINT', 'CELL'
    - ContainingCells - if True and FieldType is 'POINT', select the cells containing the
        points corresponding to the given IDs.
    - Source - If not None, specifies the sources whose elements should be selected by ID.
    - Modifier - 'ADD', 'SUBTRACT', 'TOGGLE', or None to define whether and how the selection
        should modify the existing selection.
    """
    _selectIDsHelper('HierarchicalDataIDSelectionSource', **locals())

def SelectThresholds(Thresholds=[], ArrayName='', FieldType='POINT', Source=None):
    """Select attributes in a source by thresholding on values in an associated array.

    - Thresholds - list of lower and upper threshold bounds. Attributes with associated
      values in the selected array between any set of bounds will be selected.
    - ArrayName - name of the array to threshold.
    - FieldType - atttribute to select, e.g., 'POINT' or 'CELL'
    - Source - if not set, then the selection will be on the active source
    """
    if not Source:
        Source = paraview.simple.GetActiveSource()

    selection = _createSelection('ThresholdSelectionSource', Thresholds=Thresholds, ArrayName=ArrayName, FieldType=FieldType)
    if selection:
        Source.SMProxy.SetSelectionInput(0, selection.SMProxy, 0)
    Source.UpdateVTKObjects()

def SelectLocation(Locations=[], Source=None):
    """Select points by location.

    - Locations - list of x, y, z points to select.
    - Source - if not set, then the selection will be on the active source
    """
    if not Source:
        Source = paraview.simple.GetActiveSource()

    selection = _createSelection('LocationSelectionSource', Locations=Locations, FieldType='POINT')
    if selection:
        Source.SMProxy.SetSelectionInput(0, selection.SMProxy, 0)
    Source.UpdateVTKObjects()

def QuerySelect(QueryString='', FieldType='POINT', Source=None, InsideOut=False):
    """Selection by query expression.
    - QueryString - string with NumPy-like boolean expression defining which attributes are selected
    - FieldType - atttribute to select, e.g., 'POINT' or 'CELL'
    - Source - if not set, then the selection will be on the active source
    - InsideOut - Invert the selection so that attributes that do not satisfy the expression are
      selected instead of elements that do
    """
    if not Source:
        Source = paraview.simple.GetActiveSource()

    selection = _createSelection('SelectionQuerySource', FieldType=FieldType, QueryString=QueryString, InsideOut=InsideOut)
    if selection:
        Source.SMProxy.SetSelectionInput(0, selection.SMProxy, 0)
    Source.UpdateVTKObjects()

def ClearSelection(Source=None):
    """ Clears the selection on the source passed in to the source parameter
        or the active source if no source is provided.
    """
    if Source == None:
        Source = paraview.simple.GetActiveSource()

    Source.SMProxy.SetSelectionInput(0, None, 0)
