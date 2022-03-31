## Deprecation of SPARSE_MODE in Parallel Ensight Reader

Disable the vtkPEnSightReader SPARSE_MODE that leads to crashes with some input data. The mode was automatically enabled when the number of MPI cores was superior to 12. Add a new VTK legacy CMake option allowing you to use the old behavior.
