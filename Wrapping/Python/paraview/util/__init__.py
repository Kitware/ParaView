
# These functions were present in a util.py module.
# Keeping them around for backwards compatibility
def SetOutputWholeExtent(algorithm, extent):
    """
    Convenience method to help set the WHOLE_EXTENT() in RequestInformation.
    Commonly used by programmable filters. The arguments are the algorithm
    and a tuple/list with 6 elements (xmin, xmax, ymin, ymax, zmin, zmax).

    Example use::

        import paraview.util
        # The output will be of dimensions 10, 1, 1
        paraview.util.SetOutputWholeExtent(algorithm, (0, 9, 0, 0, 0, 0)
    """
    if len(extent) != 6:
        raise "Expected a sequence of length 6"
    from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline
    algorithm.GetExecutive().GetOutputInformation(0).Set(vtkStreamingDemandDrivenPipeline.WHOLE_EXTENT(), extent[0], extent[1], extent[2],extent[3], extent[4], extent[5])

def IntegrateCell(dataset, cellId):
    """
    This functions uses vtkCellIntegrator's Integrate method that calculates
    the length/area/volume of a 1D/2D/3D cell. The calculation is exact for
    lines, polylines, triangles, triangle strips, pixels, voxels, convex
    polygons, quads and tetrahedra. All other 3D cells are triangulated
    during volume calculation. In such cases, the result may not be exact.
    """
    from paraview.modules.vtkPVVTKExtensionsDefault import vtkCellIntegrator
    return vtkCellIntegrator.Integrate(dataset, cellId)
