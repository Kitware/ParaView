if (NOT WIN32 AND PARAVIEW_USE_MPI)
  pv_plugin(AdiosStagingReader
    DESCRIPTION "Performs staging reads from simulations using ADIOS"
  )
endif()
