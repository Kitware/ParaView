# CDIReader Plugin XML tests
# these tests could run safely in serial and in parallel.
if (NOT PARAVIEW_USE_QT)
  return()
endif()

set(_paraview_add_tests_default_test_data_target ParaViewData)
ExternalData_Expand_Arguments(ParaViewData _
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/NetCDF/ts.nc}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/NetCDF/fesom.nc}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/CDISimpleRead.png,:}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/CDIUseMask.png,:}"
)

set (xml_tests
  CDISimpleRead.xml
  CDIUseMask.xml
  )

paraview_add_client_tests(
  LOAD_PLUGIN CDIReader
  BASELINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline
  TEST_DATA_TARGET ParaViewData
  PREFIX CDIReaderPlugin::pv
  TEST_SCRIPTS ${xml_tests}
  )

paraview_add_client_server_tests(
  LOAD_PLUGIN CDIReader
  BASELINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline
  TEST_DATA_TARGET ParaViewData
  PREFIX CDIReaderPlugin::pvcs
  TEST_SCRIPTS ${xml_tests}
  )
