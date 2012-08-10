vtk_module(vtkPVPythonSupport
  GROUPS
    ParaViewPython
  DEPENDS
    vtkCommonCore
    vtksys
  EXCLUDE_FROM_WRAPPING
)
