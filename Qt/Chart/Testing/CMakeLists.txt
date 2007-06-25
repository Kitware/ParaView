
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
ADD_EXECUTABLE(pqChartTest QTestApp.cxx ${Tests})
TARGET_LINK_LIBRARIES(pqChartTest QtChart ${QT_LIBRARIES})
  
FOREACH(test ${MyTests})
  ADD_TEST(pqChart${test} "${EXECUTABLE_OUTPUT_PATH}/pqChartTest" ${test} --exit)
ENDFOREACH(test ${MyTests})

