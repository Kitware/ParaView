if (PARAVIEW_USE_PISTON)
  pv_plugin(PistonPlugin
    DESCRIPTION "LANL Piston GPU accelerated filters"
    DEFAULT_ENABLED)
endif()
