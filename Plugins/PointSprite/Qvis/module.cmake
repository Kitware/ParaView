if (PARAVIEW_BUILD_QT_GUI)
  vtk_module(vtkQvis
    DEPENDS
      vtkCommonCore
    EXCLUDE_FROM_WRAPPING
    TEST_LABELS PARAVIEW)
endif()
