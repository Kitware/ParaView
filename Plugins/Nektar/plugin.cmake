if (PARAVIEW_USE_MPI AND NOT WIN32)
  pv_plugin(Nektar
    DESCRIPTION "Nektar Reader Plugin"
    PLUGIN_NAMES pvNektarReader
  )
endif()
