
PROJECT(pqChartTest)

SET(MyTests
  ColorMap
  Histogram
  HistogramSelection
  HistogramInteraction
  LineChart
  )

INCLUDE_DIRECTORIES(${QtChart_SOURCE_DIR} ${QtChart_BINARY_DIR})
CREATE_TEST_SOURCELIST(Tests pqChartTest.cxx ${MyTests}) 
ADD_EXECUTABLE(pqChartTest MACOSX_BUNDLE ${Tests})
TARGET_LINK_LIBRARIES(pqChartTest QtChart ${QT_LIBRARIES})
  
MACRO(GET_TEST_BINARY in out)
  IF(APPLE)
    SET(${out} "${EXECUTABLE_OUTPUT_PATH}/${in}.app/Contents/MacOS/${in}")
  ELSE(APPLE)
    SET(${out} "${EXECUTABLE_OUTPUT_PATH}/${in}")
  ENDIF(APPLE)
ENDMACRO(GET_TEST_BINARY)

GET_TEST_BINARY(pqChartTest TEST_BINARY)

FOREACH(test ${MyTests})
  ADD_TEST(pqChart${test} ${TEST_BINARY} ${test} --exit)
ENDFOREACH(test ${MyTests})

