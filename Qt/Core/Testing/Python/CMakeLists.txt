if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
  QT5_WRAP_CPP(MOC_SRCS pqFileDialogTest.h)
else ()
  QT4_WRAP_CPP(MOC_SRCS pqFileDialogTest.h)
endif ()
vtk_module_test_executable(pqFileDialogTest
  pqFileDialogTest.cxx pqFileDialogTest.h ${MOC_SRCS})
if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
  set_target_properties(pqFileDialogTest PROPERTIES
    COMPILE_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
endif ()

add_test(
  NAME pqFileDialogTest
  COMMAND pqFileDialogTest -dr
         "--test-directory=${PARAVIEW_TEST_DIR}"
         "--test-script=${CMAKE_CURRENT_SOURCE_DIR}/pqFileDialogTest.py"
         --exit
)
set_tests_properties(pqFileDialogTest PROPERTIES LABELS "PARAVIEW")
