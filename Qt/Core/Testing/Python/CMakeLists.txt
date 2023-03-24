find_package("Qt${PARAVIEW_QT_MAJOR_VERSION}" REQUIRED QUIET COMPONENTS Core Widgets)
set(CMAKE_AUTOMOC 1)
vtk_module_test_executable(pqFileDialogTest
  pqFileDialogTest.cxx pqFileDialogTest.h)
target_link_libraries(pqFileDialogTest LINK_PRIVATE "Qt${PARAVIEW_QT_MAJOR_VERSION}::Core" "Qt${PARAVIEW_QT_MAJOR_VERSION}::Widgets")

add_test(
  NAME pqFileDialogTest
  COMMAND pqFileDialogTest
         --dr
         "--test-script=${CMAKE_CURRENT_SOURCE_DIR}/pqFileDialogTest.py"
         --exit
)
set_tests_properties(pqFileDialogTest PROPERTIES LABELS "ParaView")
