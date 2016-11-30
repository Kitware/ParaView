SET(MyTests
  FlatTreeView
  Animation
  )

SET(MocSources
  SignalCatcher.h)

if(NOT ${CMAKE_Cxx_Fortran_COMPILER_ID} STREQUAL "Intel")
  # QtTest fails to compile on ICC. Hence we don't add this test on
  # Intel compilers.
  list(APPEND MyTests pqTextEditTest)
  list(APPEND MocSources pqTextEditTest.h)
endif()

include(ParaViewQt)
pv_find_package_qt(qt_targets REQUIRED QUIET
                QT4_COMPONENTS QtCore QtGui QtTest
                QT5_COMPONENTS Core Widgets Test)
pv_qt_wrap_cpp(Tests_MOC_SOURCES ${MocSources})

ADD_DEFINITIONS(-DQT_GUI_LIB)
INCLUDE_DIRECTORIES(${QtWidgets_SOURCE_DIR} ${QtWidgets_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})
CREATE_TEST_SOURCELIST(Tests pqWidgetsTest.cxx ${MyTests})

vtk_module_test_executable(pqWidgetsTest QTestApp.cxx ${Tests} ${Tests_MOC_SOURCES})
target_link_libraries(pqWidgetsTest LINK_PRIVATE ${qt_targets})

FOREACH(test ${MyTests})
  ADD_TEST(
    NAME pqWidgets${test}
    COMMAND pqWidgetsTest ${test} --exit)
  set_tests_properties(pqWidgets${test} PROPERTIES LABELS "PARAVIEW")
ENDFOREACH()

if(NOT ${CMAKE_Cxx_Fortran_COMPILER_ID} STREQUAL "Intel")
  # since serial since this relies on focus.
  set_tests_properties(pqWidgetspqTextEditTest PROPERTIES RUN_SERIAL ON)
endif()
