if(PARAVIEW_USE_MPI)
  pv_plugin(GenericIOReader
    DESCRIPTION "GenericIO Reader for HACC data"
    DEFAULT_ENABLED)
endif()
