find_package(Qt5 REQUIRED QUIET COMPONENTS Core Widgets)
set(CMAKE_AUTOMOC 1)
vtk_module_test_executable(pqFileDialogTest
  pqFileDialogTest.cxx pqFileDialogTest.h)
target_link_libraries(pqFileDialogTest LINK_PRIVATE Qt5::Core Qt5::Widgets)

add_test(
  NAME pqFileDialogTest
  COMMAND pqFileDialogTest -dr
         "--test-directory=${PARAVIEW_TEST_DIR}"
         "--test-script=${CMAKE_CURRENT_SOURCE_DIR}/pqFileDialogTest.py"
         --exit
)
set_tests_properties(pqFileDialogTest PROPERTIES LABELS "ParaView")
