if (PARAVIEW_USE_MPI)
  add_definitions("-DMPICH_IGNORE_CXX_SEEK")
endif()
vtk_module(vtkPVClientServerCoreDefault
  DEPENDS
    vtkPVClientServerCoreRendering
    vtkPVVTKExtensionsDefault
  PRIVATE_DEPENDS
    vtksys
    vtkIOInfovis
  TEST_DEPENDS
    vtkTestingCore
  TEST_LABELS
    PARAVIEW
  KIT
    vtkPVClientServer
)
