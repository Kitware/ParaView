include(ParaViewQt)
pv_find_package_qt(qt_targets REQUIRED QUIET
                QT4_COMPONENTS QtCore QtGui
                QT5_COMPONENTS Core Widgets)
pv_qt_wrap_cpp(MOC_SRCS BasicApp.h)
vtk_module_test_executable(pqCoreBasicApp BasicApp.cxx BasicApp.h ${MOC_SRCS})
target_link_libraries(pqCoreBasicApp LINK_PRIVATE ${qt_targets})
ExternalData_add_test(ParaViewData
  NAME pqCoreBasicApp
  COMMAND pqCoreBasicApp
          -dr "--test-directory=${PARAVIEW_TEST_DIR}"
          "--test-baseline=DATA{${PARAVIEW_TEST_BASELINE_DIR}/pqCoreBasicApp.png}"
          --exit
  )
set_tests_properties(pqCoreBasicApp PROPERTIES LABELS "PARAVIEW")
