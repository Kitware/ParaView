import os

if os.name == "posix":
    from libvtkCommonPython import *
else:
    from vtkCommonPython import *

def IntegrateCell(dataset, cellId):
    """
    This functions uses vtkCellIntegrator's Integrate method that calculates
    the length/area/volume of a 1D/2D/3D cell. The calculation is exact for
    lines, polylines, triangles, triangle strips, pixels, voxels, convex
    polygons, quads and tetrahedra. All other 3D cells are triangulated
    during volume calculation. In such cases, the result may not be exact.
    """
    
    return vtkCellIntegrator.Integrate(dataset, cellId)

