if (BUILD_SHARED_LIBS)

  ExternalData_Expand_Arguments("ParaViewData" _
    "DATA{Data/Baseline/SaveStateAndScreenshot.png}"
    "DATA{Data/Baseline/SaveStateAndScreenshotActiveView.png}"
    "DATA{Data/Baseline/SaveStateAndScreenshotActiveView2.png}")

  paraview_add_client_tests(
    LOAD_PLUGIN "SaveStateAndScreenshot"
    TEST_SCRIPTS
      "${CMAKE_CURRENT_SOURCE_DIR}/SaveStateAndScreenshot.xml"
      "${CMAKE_CURRENT_SOURCE_DIR}/SaveStateAndScreenshotActiveView.xml")
endif()
