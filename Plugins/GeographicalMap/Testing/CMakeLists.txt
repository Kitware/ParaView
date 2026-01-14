include(ParaViewTesting)

if (BUILD_SHARED_LIBS)
  find_package(OpenSSL COMPONENTS SSL QUIET)

  if (OpenSSL_FOUND)
    ExternalData_Expand_Arguments("ParaViewData" _
      "DATA{Data/Baseline/LoadOSMTile.png}")

    set(_pv_ext_baseline "${CMAKE_BINARY_DIR}/ExternalData/Plugins/GeographicalMap/Testing/Data/Baseline")

    paraview_add_client_tests(
      LOAD_PLUGIN GeographicalMap
      PLUGIN_PATH $<TARGET_FILE_DIR:GeographicalMap>
      ENVIRONMENT "PARAVIEW_TEST_BASELINE_DIR=${_pv_ext_baseline}"
      TEST_DATA_TARGET ParaViewData
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LoadOSMTile.xml)
  else()
    message(STATUS "GeographicalMap tests skipped: OpenSSL (SSL) not found")
  endif()
endif()
