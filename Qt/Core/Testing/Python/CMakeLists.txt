include(ParaViewQt)
pv_find_package_qt(qt_targets REQUIRED QUIET
                QT4_COMPONENTS QtCore QtGui
                QT5_COMPONENTS Core Widgets)
pv_qt_wrap_cpp(MOC_SRCS pqFileDialogTest.h)
vtk_module_test_executable(pqFileDialogTest
  pqFileDialogTest.cxx pqFileDialogTest.h ${MOC_SRCS})
target_link_libraries(pqFileDialogTest LINK_PRIVATE ${qt_targets})

add_test(
  NAME pqFileDialogTest
  COMMAND pqFileDialogTest -dr
         "--test-directory=${PARAVIEW_TEST_DIR}"
         "--test-script=${CMAKE_CURRENT_SOURCE_DIR}/pqFileDialogTest.py"
         --exit
)
set_tests_properties(pqFileDialogTest PROPERTIES LABELS "PARAVIEW")
