set(Module_SRCS
  vtkSIPythonSourceProxy.cxx
  vtkPVPythonAlgorithmPlugin.cxx
  )

vtk_module_library(vtkPVPythonAlgorithm ${Module_SRCS})
