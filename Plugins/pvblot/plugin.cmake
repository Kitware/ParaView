if (PARAVIEW_ENABLE_PYTHON AND FALSE)
  # blot is currently not working. We'll fix it when needed.
  pv_plugin(pvblot
    DESCRIPTION "PVBLOT Plugin"
    DEFAULT_ENABLED)
endif()
