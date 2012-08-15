QT4_WRAP_CPP(MOC_SRCS pqFileDialogTest.h)
vtk_module_test_executable(pqFileDialogTest
  pqFileDialogTest.cxx pqFileDialogTest.h ${MOC_SRCS})

add_test(
  NAME pqFileDialogTest
  COMMAND pqFileDialogTest -dr
         "--test-directory=${PARAVIEW_TEST_DIR}"
         "--test-script=${CMAKE_CURRENT_SOURCE_DIR}/pqFileDialogTest.py"
         --exit
)
set_tests_properties(pqFileDialogTest PROPERTIES LABELS "PARAVIEW")
