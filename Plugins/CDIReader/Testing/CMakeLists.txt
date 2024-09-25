# CDIReader Plugin XML tests
# these tests could run safely in serial and in parallel.
if (NOT PARAVIEW_USE_QT)
  return()
endif()

set(_paraview_add_tests_default_test_data_target ParaViewData)
ExternalData_Expand_Arguments(ParaViewData _
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/NetCDF/edges.nc}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/NetCDF/ts.nc}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/NetCDF/fesom.nc}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/CDISimpleRead_A.png,:}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/CDISimpleRead_B.png,:}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/CDISimpleRead_C.png,:}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/CDISimpleRead_D.png,:}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/CDISimpleRead_E.png,:}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/CDIUseMask_A.png,:}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/CDIUseMask_B.png,:}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/CDIUseMask_C.png,:}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/CDIUseMask_D.png,:}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/CDIUseMask_E.png,:}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/CDIUseMask_F.png,:}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/CDIDimensionsTest_A.png,:}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/CDIDimensionsTest_B.png,:}"
)

set (xml_tests
  CDISimpleRead.xml
  CDIUseMask.xml
  CDIDimensionsTest.xml
)

set(CDISimpleRead_METHOD LOOSE_VALID)

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
