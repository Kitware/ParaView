if (PARAVIEW_BUILD_QT_GUI AND PARAVIEW_ENABLE_PYTHON)

  pv_plugin(PythonQtPlugin
    DESCRIPTION "PythonQt Plugin"
    )

endif()
