if (PARAVIEW_DATA_ROOT)
  SET(BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline)

  if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
    QT5_WRAP_CPP(MOC_SRCS BasicApp.h)
  else ()
    QT4_WRAP_CPP(MOC_SRCS BasicApp.h)
  endif ()
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
