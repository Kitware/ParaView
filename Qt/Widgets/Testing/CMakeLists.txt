
PROJECT(pqWidgetsTest)

SET(MyTests
  FlatTreeView
  )

INCLUDE_DIRECTORIES(${QtWidgets_SOURCE_DIR} ${QtWidgets_BINARY_DIR})
CREATE_TEST_SOURCELIST(Tests pqWidgetsTest.cxx ${MyTests}) 
ADD_EXECUTABLE(pqWidgetsTest MACOSX_BUNDLE QTestApp.cxx ${Tests})
TARGET_LINK_LIBRARIES(pqWidgetsTest pqWidgets ${QT_LIBRARIES})
  
MACRO(GET_TEST_BINARY in out)
  IF(APPLE)
    SET(${out} "${EXECUTABLE_OUTPUT_PATH}/${in}.app/Contents/MacOS/${in}")
  ELSE(APPLE)
    SET(${out} "${EXECUTABLE_OUTPUT_PATH}/${in}")
  ENDIF(APPLE)
ENDMACRO(GET_TEST_BINARY)

GET_TEST_BINARY(pqWidgetsTest TEST_BINARY)

FOREACH(test ${MyTests})
  ADD_TEST(pqWidgets${test} ${TEST_BINARY} ${test} --exit)
ENDFOREACH(test ${MyTests})

