if (PARAVIEW_USE_QT AND BUILD_SHARED_LIBS)

  ExternalData_Expand_Arguments("ParaViewData" _
    "DATA{Data/Baseline/SaveStateAndScreenshot.png}")

  paraview_add_client_tests(
    LOAD_PLUGIN "SaveStateAndScreenshot"
    TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/SaveStateAndScreenshot.xml")
endif()
