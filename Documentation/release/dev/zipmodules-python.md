# ZIP archives for Python packages and modules in static builds

In static builds of ParaView with Python enabled, we package all Python modules
and packages into ZIP archives. This helps reduce the file accesses needed to import
ParaView's Python modules which can have a negative impact on HPC systems when running
at scale. With ZIP archives, there are just two files `_paraview.zip` and `_vtk.zip` that
need to opened to import all ParaView's (and VTK's) Python modules.
