if (PARAVIEW_BUILD_QT_GUI AND PARAVIEW_ENABLE_COSMOTOOLS)
  ExternalData_Expand_Arguments("ParaViewData" _
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/adaptive-cosmo/,REGEX:.*}"
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/multiresolution-streaming/,REGEX:.*}"
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/,REGEX:StreamingParticles[12].*\\.png}"
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/,REGEX:NewGenericIOStreaming[1-5].*\\.png}")

  set(module_tests
    StreamingParticles.xml
    NewGenericIOStreaming.xml)

  paraview_add_client_tests(
    LOAD_PLUGIN "StreamingParticles"
    TEST_DATA_TARGET ParaViewData
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_SCRIPTS ${module_tests})
  paraview_add_client_server_tests(
    LOAD_PLUGIN "StreamingParticles"
    TEST_DATA_TARGET ParaViewData
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_SCRIPTS ${module_tests})
endif ()
