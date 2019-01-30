find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
set(CMAKE_AUTOMOC 1)
vtk_module_test_executable(pqCoreBasicApp BasicApp.cxx BasicApp.h)
target_link_libraries(pqCoreBasicApp PRIVATE Qt5::Core Qt5::Widgets)
ExternalData_add_test("${_vtk_build_TEST_DATA_TARGET}"
  NAME pqCoreBasicApp
  COMMAND pqCoreBasicApp
          -dr "--test-directory=${_vtk_build_TEST_OUTPUT_DIRECTORY}"
          "--test-baseline=DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/Baseline/pqCoreBasicApp.png}"
          --exit
  )
