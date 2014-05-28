if (PARAVIEW_ENABLE_QT_GUI OR PARAVIEW_ENABLE_QT_SUPPORT)
  vtk_module(pqDeprecated
    DEPENDS
      pqApplicationComponents
    EXCLUDE_FROM_WRAPPING
    TEST_LABELS
      PARAVIEW
  )
endif ()
