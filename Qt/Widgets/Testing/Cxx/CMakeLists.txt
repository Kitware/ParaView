SET(MyTests
  CTKSliders
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


IF(PARAVIEW_QT_VERSION VERSION_GREATER "4")
  QT5_WRAP_CPP(Tests_MOC_SOURCES ${MocSources})
ELSE()
  QT4_WRAP_CPP(Tests_MOC_SOURCES ${MocSources})
ENDIF()

ADD_DEFINITIONS(-DQT_GUI_LIB)
INCLUDE_DIRECTORIES(${QtWidgets_SOURCE_DIR} ${QtWidgets_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})
CREATE_TEST_SOURCELIST(Tests pqWidgetsTest.cxx ${MyTests})

vtk_module_test_executable(pqWidgetsTest QTestApp.cxx ${Tests} ${Tests_MOC_SOURCES})
target_link_libraries(pqWidgetsTest LINK_PRIVATE ${QT_QTTEST_LIBRARY})

IF(PARAVIEW_QT_VERSION VERSION_GREATER "4")
  SET_TARGET_PROPERTIES(pqWidgetsTest PROPERTIES
    COMPILE_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
ENDIF()
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
