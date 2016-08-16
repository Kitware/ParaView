set (__dependencies)

if (PARAVIEW_USE_MPI)
  list (APPEND __dependencies vtkParallelMPI)
endif()

if (PARAVIEW_ENABLE_PYTHON)
  list(APPEND __dependencies
      vtkPythonInterpreter
      vtkFiltersProgrammable)
endif ()

set (__compile_dependencies)
if (PARAVIEW_ENABLE_PYTHON AND PARAVIEW_USE_MPI)
  list(APPEND __compile_dependencies vtkmpi4py)
endif()

vtk_module(vtkPVClientServerCoreCore
  GROUPS
    ParaViewCore
  DEPENDS
    vtkFiltersExtraction
    vtkFiltersParallel
    # Explicitely list (rather than transiently through
    # vtkPVVTKExtensionsCore) because it allows us to turn of wrapping
    # of vtkPVVTKExtensionsCore off but still satisfy API dependcy.
    vtkPVCommon
    vtkPVVTKExtensionsCore
    vtkPVCommon
    ${__dependencies}
    vtkCommonSystem
    vtkIOLegacy
    vtkCommonCore
  PRIVATE_DEPENDS
    vtksys
  COMPILE_DEPENDS
  # This ensures that CS wrappings will be generated 
    vtkUtilitiesWrapClientServer
    ${__compile_dependencies}
  TEST_LABELS
    PARAVIEW
  KIT
    vtkPVClientServer
)
