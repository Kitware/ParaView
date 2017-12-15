if(PARAVIEW_USE_MPI AND NOT WIN32)
  pv_plugin(GenericIOReader
    DESCRIPTION "GenericIO Reader for HACC data"
    DEFAULT_ENABLED)
endif()
