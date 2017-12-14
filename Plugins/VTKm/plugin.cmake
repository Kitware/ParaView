if(PARAVIEW_USE_VTKM)
  pv_plugin(VTKmFilters
    DESCRIPTION "VTKm many-core filters"
    DEFAULT_ENABLED
    )
endif()
