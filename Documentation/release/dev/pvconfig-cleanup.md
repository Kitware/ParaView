# pvconfig-cleanup

* The `vtkPVConfig.h` header no longer provides feature details like whether
  MPI, Python, or various VisIt readers are enabled. Instead, CMake logic
  should be used to detect whether the metafeatures are enabled (VisIt readers
  are not exposed currently).
