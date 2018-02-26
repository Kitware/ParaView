set (__dependencies)

if (PARAVIEW_USE_MPI)
  list (APPEND __dependencies vtkParallelMPI)
endif()

if (PARAVIEW_ENABLE_PYTHON)
  list(APPEND __dependencies
      vtkPythonInterpreter
      vtkFiltersProgrammable)
endif ()

if (PARAVIEW_ENABLE_NVPIPE)
  list(APPEND __dependencies vtknvpipe)
endif ()

vtk_module(vtkPVClientServerCoreCore
  GROUPS
    ParaViewCore
  DEPENDS
    vtkFiltersExtraction
    vtkFiltersParallel
    # Explicitly list (rather than transiently through
    # vtkPVVTKExtensionsCore) because it allows us to turn of wrapping
    # of vtkPVVTKExtensionsCore off but still satisfy API dependency.
    vtkPVCommon
    vtkPVVTKExtensionsCore
    vtkPVCommon
    ${__dependencies}
    vtkCommonSystem
    vtkIOLegacy
    vtkCommonCore
    vtkPVVTKExtensionsSIL
  PRIVATE_DEPENDS
    vtksys
    vtkCommonMisc
  COMPILE_DEPENDS
  # This ensures that CS wrappings will be generated 
    vtkUtilitiesWrapClientServer
  TEST_LABELS
    PARAVIEW
  KIT
    vtkPVClientServer
)
