set(Module_SRCS
  vtkBoundedPlaneSource.cxx
  vtkBoundedVolumeSource.cxx
)
vtk_module_library(${vtk-module} ${Module_SRCS})
