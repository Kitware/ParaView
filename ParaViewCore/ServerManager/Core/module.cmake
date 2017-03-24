set (__dependencies)
if (PARAVIEW_ENABLE_PYTHON)
  list (APPEND __dependencies vtkPythonInterpreter)
endif ()

vtk_module(vtkPVServerManagerCore
  GROUPS
    ParaViewCore
  DEPENDS
    # Explicitely list (rather than transiently through
    # vtkPVServerImplementationCore) because it allows us to turn of wrapping
    # of vtkPVServerImplementationCore off but still satisfy API dependcy.
    vtkCommonCore
    vtkPVServerImplementationCore
    vtkjsoncpp
  PRIVATE_DEPENDS
    vtksys
    vtkpugixml
    ${__dependencies}
  TEST_LABELS
    PARAVIEW
  TEST_DEPENDS
    vtkPVServerManagerApplication
  KIT
    vtkPVServerManager

)
unset (__dependencies)
