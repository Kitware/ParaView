set(module_tests
  VTKmClip.xml
  VTKmContour.xml
  VTKmExternalFaces.xml
  VTKmGradient.xml
  VTKmGradientUG.xml
  VTKmThreshold.xml
)

if (PARAVIEW_USE_QT AND BUILD_SHARED_LIBS)
  paraview_add_client_tests(
    LOAD_PLUGIN "VTKmFilters"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${module_tests}
  )
endif ()

if (PARAVIEW_USE_PYTHON)
  paraview_add_test_pvbatch(
    NO_DATA NO_VALID
    LoadVTKmFilterPlugin.py)
endif()
