vtk_module(vtkPVVTKExtensionsPoints
  DEPENDS
    vtkFiltersPoints
  PRIVATE_DEPENDS
    vtkFiltersCore
  TEST_LABELS
    PARAVIEW
  KIT
    vtkPVExtensions
)

set_property(GLOBAL PROPERTY
  vtkPVVTKExtensionsPoints_SERVERMANAGER_XMLS
    ${CMAKE_CURRENT_LIST_DIR}/points.xml
    )
