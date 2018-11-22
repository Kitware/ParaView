"""This is the paraview.vtk module."""
import sys

if sys.version_info <= (3,4):
    # imp is deprecated in 3.4
    import imp, importlib

    # import vtkmodules package.
    vtkmodules_m = importlib.import_module('vtkmodules')

    # import paraview.pv-vtk-all
    all_m = importlib.import_module('paraview.pv-vtk-all')

    # create a clone of the `vtkmodules.all` module.
    vtk_m = imp.new_module(__name__)
    for key in dir(all_m):
        if not hasattr(vtk_m, key):
            setattr(vtk_m, key, getattr(all_m, key))

    # make the clone of `vtkmodules.all` act as a package at the same location
    # as vtkmodules. This ensures that importing modules from within the vtkmodules package
    # continues to work.
    vtk_m.__path__ = vtkmodules_m.__path__
    # replace old `vtk` module with this new package.
    sys.modules[__name__] = vtk_m

else:
    import importlib.util
    # import vtkmodules.all
    all_spec = importlib.util.find_spec('paraview.pv-vtk-all')
    all_m = importlib.util.module_from_spec(all_spec)
    all_spec.loader.exec_module(all_m)

    # import paraview.pv-vtk-all
    vtkmodules_spec = importlib.util.find_spec('vtkmodules')
    vtkmodules_m = importlib.util.module_from_spec(vtkmodules_spec)
    vtkmodules_spec.loader.exec_module(vtkmodules_m)

    # add attributes from pv-vtk-all
    for key in dir(all_m):
        if not hasattr(vtkmodules_m, key):
            setattr(vtkmodules_m, key, getattr(all_m, key))

    # replace old `vtk` module with the modified `vtkmodules_m` package.
    sys.modules[__name__] = vtkmodules_m
