if (PARAVIEW_ENABLE_QT_GUI OR PARAVIEW_ENABLE_QT_SUPPORT)
  vtk_module(pqApplicationComponents
    GROUPS
      ParaViewQt
    DEPENDS
      pqComponents
      vtkGUISupportQt
    PRIVATE_DEPENDS
      vtksys
    EXCLUDE_FROM_WRAPPING
    TEST_LABELS
      PARAVIEW
  )
endif ()
