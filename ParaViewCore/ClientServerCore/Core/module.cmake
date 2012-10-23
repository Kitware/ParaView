set (__dependencies)

if (PARAVIEW_USE_MPI)
  list (APPEND __dependencies vtkParallelMPI)

  # needed for mpich
  add_definitions("-DMPICH_IGNORE_CXX_SEEK")
endif()

if (PARAVIEW_ENABLE_PYTHON)
  list(APPEND __dependencies
      vtkPVPythonSupport
      vtkFiltersProgrammable)
endif (PARAVIEW_ENABLE_PYTHON)

vtk_module(vtkPVClientServerCoreCore
  GROUPS
    ParaViewCore
  DEPENDS
    vtkFiltersExtraction
    # Explicitely list (rather than transiently through
    # vtkPVVTKExtensionsCore) because it allows us to turn of wrapping
    # of vtkPVVTKExtensionsCore off but still satisfy API dependcy.
    vtkPVCommon
    vtkPVVTKExtensionsCore
    ${__dependencies}
  COMPILE_DEPENDS
  # This ensures that CS wrappings will be generated 
    vtkUtilitiesWrapClientServer
  TEST_LABELS
    PARAVIEW
)
