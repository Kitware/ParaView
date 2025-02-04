This example illustrate how to create a complex, multi-file, python plugin.

It highlights the following points:
* How to import a custom module from the plugin file: `complex_python_plugin.py`. This is the file to import as plugin.
* How to create Proxies: `my_module/pv_my_python_source.py`, `my_module/pv_my_python_filter.py`
* How to choose what to expose in ParaView: `my_module/__init__.py`
* How to use pure VTK code alongside to Proxies definitions: `my_module/vtk_internal_filter.py`

## Concept

A Python plugin is a way to combine a VTK Python filter and its associated server manager XML in the same file.
It relies on 2 key features:
- the `VTKPythonAlgorithmBase` allows to subclass vtkAlgorithm in Python
- a list of Python decorators to inject XML information in a clean way directly above
the object (class / method) definition.

## A matter of paths

When developping complex code, it is a good practice to split
the different objects and features into dedicated files.
But to load a ParaView plugin, one should select *a single file*.
With compiled c++ plugin, this is easy: the build step generates
a main library file.

With Python plugin, it is still possible. But one should properly
manage the paths so ParaView can find custom modules.

See the `complex_python_plugin.py` file.

## Python modules in use

The entry point of a Python plugin is the `paraview.util.vtkAlgorithm` module.
It includes the VTK base class and utilities to create the Proxies/Properties.
This is all what you need to create a Python plugin. The following are just for completness of documentation.

Internally, the decorators used to mimic the server manager XML files are defined
in  `paraview.detail.pythonalgorithm` module.

The base class for VTK Python algorithm, `VTKPythonAlgorithmBase` is under `vtkmodules.util.vtkAlgorithm`.

Also illustrated in this example, but not as the main goal: the numpy wrapping comes from
`vtkmodules.numpy_interface`, containing the `dataset_adapter` and the `algorithms` modules.
