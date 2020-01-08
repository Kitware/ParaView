set(module_tests
  SLACTools.xml
)

if (PARAVIEW_USE_QT AND BUILD_SHARED_LIBS)
  ExternalData_Expand_Arguments("ParaViewData" _
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/,REGEX:.*}"
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/SLAC/pic-example/,REGEX:.*}"
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/SLAC/pillbox/,REGEX:.*}")

  set(SLACTools_THRESHOLD 70)

  paraview_add_client_tests(
    LOAD_PLUGIN "SLACTools"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${MODULE_TESTS}
  )
endif ()
