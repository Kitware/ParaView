if (PARAVIEW_DATA_ROOT)
  SET(BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline)

  QT4_WRAP_CPP(MOC_SRCS BasicApp.h)
  vtk_module_test_executable(pqCoreBasicApp BasicApp.cxx BasicApp.h ${MOC_SRCS})
  add_test(
    NAME pqCoreBasicApp 
    COMMAND pqCoreBasicApp
            -dr "--test-directory=${PARAVIEW_TEST_DIR}"
            "--test-baseline=${BASELINE_DIR}/pqCoreBasicApp.png"
            --exit
    )
  set_tests_properties(pqCoreBasicApp PROPERTIES LABELS "PARAVIEW")
endif()
