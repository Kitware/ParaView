# Improved ability to add Python-based algorithms via Plugin

Support for developing filters, readers, writers in Python and distributing them
as plugins has been improved. `paraview.util.vtkAlgorithm` module provides
decorators to expose `VTKPythonAlgorithmBase`-based VTK algorithms in ParaView.
A python module that provides such decorated algorithms can be directly loaded
in ParaView as plugins.
