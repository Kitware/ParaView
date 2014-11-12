
CGNS Reader ParaView Plugin
===========================

This is a small plugin to load [CGNS](http://cgns.sourceforge.net/) files.

This is inspired by the VisIt CGNS reader originally written by
B. Whitlock. But it relies on the low level CGNS API to load DataSet
and try to reduce memory footprint.

Features:
  * Multi-block dataset are created for ustructured grids
and structured meshes stored in binary CGNS file format.
  * Basic support for CGNS vectors (Velocity, ...) following SIDS naming scheme
  * Basic time support with either one time step per file or one file for all timesteps (not tested)
  * Basic and not efficient parallelism
  * When compiled with sizeof(cgsize_t) == sizeof(vtkIdType), it prevents buffering unstructured mesh connectivity.
  * On single processor, boundary patches can be loaded for unstructured grid
