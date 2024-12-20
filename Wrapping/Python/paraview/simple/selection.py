from paraview import servermanager

from paraview.selection import (
    SelectionProxy,
    CreateSelection,
    SelectSurfacePoints,
    SelectSurfaceCells,
    SelectSurfaceBlocks,
    SelectPointsThrough,
    SelectCellsThrough,
    SelectGlobalIDs,
    SelectPedigreeIDs,
    SelectIDs,
    SelectCompositeDataIDs,
    SelectHierarchicalDataIDs,
    SelectThresholds,
    SelectLocation,
    QuerySelect,
)
from paraview.simple.session import GetActiveSource

__all__ = [
    "SelectionProxy",
    "CreateSelection",
    "SelectSurfacePoints",
    "SelectSurfaceCells",
    "SelectSurfaceBlocks",
    "SelectPointsThrough",
    "SelectCellsThrough",
    "SelectGlobalIDs",
    "SelectPedigreeIDs",
    "SelectIDs",
    "SelectCompositeDataIDs",
    "SelectHierarchicalDataIDs",
    "SelectThresholds",
    "SelectLocation",
    "QuerySelect",
    "SelectCells",
    "SelectPoints",
    "ClearSelection",
]


# ==============================================================================
# Selection Management
# ==============================================================================
def _select(seltype, query=None, proxy=None):
    """Function to help create query selection sources.

    :param seltype: Selection type
    :type seltype: str
    :param query: Query expression that defines the selection. Optional, defaults
        to expression that selects all cells.
    :type query: str
    :param proxy: The source proxy to select from. Optional, defaults to the
        active source.
    :type proxy: Source proxy.
    :return: The created selection source.
    :rtype: :class:`servermanager.sources.SelectionQuerySource`"""
    if not proxy:
        proxy = GetActiveSource()
    if not proxy:
        raise RuntimeError("No active source was found.")

    if not query:
        # This ends up being true for all cells.
        query = "id >= 0"

    from paraview.vtk import vtkDataObject

    # Note, selSource is not registered with the proxy manager.
    selSource = servermanager.sources.SelectionQuerySource()
    if seltype.lower() == "point":
        elementType = vtkDataObject.POINT
    elif seltype.lower() == "cell":
        elementType = vtkDataObject.CELL
    else:
        elementType = seltype
    selSource.ElementType = elementType
    selSource.QueryString = str(query)
    proxy.SMProxy.SetSelectionInput(proxy.Port, selSource.SMProxy, 0)
    return selSource


# -----------------------------------------------------------------------------


def SelectCells(query=None, proxy=None):
    """Select cells satisfying the query.

    :param query: The selection query. If `None`, then all cells are
        selected.
    :type query: str
    :param proxy: The source proxy to select from. Optional, defaults to the
        active source.
    :type proxy: Source proxy
    :return: Selection source
    :rtype: :class:`servermanager.sources.SelectionQuerySource`"""
    return _select("CELL", query, proxy)


# -----------------------------------------------------------------------------


def SelectPoints(query=None, proxy=None):
    """Select points satisfying the query.

    :param query: The selection query. If `None`, then all points are
        selected.
    :type query: str
    :param proxy: The source proxy to select from. Optional, defaults to the
        active source.
    :type proxy: Source proxy
    :return: Selection source
    :rtype: :class:`servermanager.sources.SelectionQuerySource`"""
    return _select("POINT", query, proxy)


# -----------------------------------------------------------------------------


def ClearSelection(proxy=None):
    """Clears the selection on the active source.

    :param proxy: Source proxy whose selection should be cleared.
    :type proxy: Source proxy"""
    if not proxy:
        proxy = GetActiveSource()
    if not proxy:
        raise RuntimeError("No active source was found.")
    proxy.SMProxy.SetSelectionInput(proxy.Port, None, 0)
