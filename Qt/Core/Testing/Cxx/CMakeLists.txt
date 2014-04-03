if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
  QT5_WRAP_CPP(MOC_SRCS BasicApp.h)
else ()
  QT4_WRAP_CPP(MOC_SRCS BasicApp.h)
endif ()
vtk_module_test_executable(pqCoreBasicApp BasicApp.cxx BasicApp.h ${MOC_SRCS})
if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
  set_target_properties(pqCoreBasicApp PROPERTIES
    COMPILE_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
endif ()
ExternalData_add_test(ParaViewData
  NAME pqCoreBasicApp
  COMMAND pqCoreBasicApp
          -dr "--test-directory=${PARAVIEW_TEST_DIR}"
          "--test-baseline=DATA{${PARAVIEW_TEST_BASELINE_DIR}/pqCoreBasicApp.png}"
          --exit
  )
set_tests_properties(pqCoreBasicApp PROPERTIES LABELS "PARAVIEW")
