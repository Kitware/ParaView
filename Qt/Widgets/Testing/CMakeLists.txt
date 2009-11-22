
PROJECT(pqWidgetsTest)

SET(MyTests
  FlatTreeView
  Animation
  )

INCLUDE_DIRECTORIES(${QtWidgets_SOURCE_DIR} ${QtWidgets_BINARY_DIR})
CREATE_TEST_SOURCELIST(Tests pqWidgetsTest.cxx ${MyTests}) 
ADD_EXECUTABLE(pqWidgetsTest QTestApp.cxx ${Tests})
TARGET_LINK_LIBRARIES(pqWidgetsTest pqWidgets ${QT_LIBRARIES}
  ${QT_QTNETWORK_LIBRARY})
  
FOREACH(test ${MyTests})
  ADD_TEST(pqWidgets${test} "${EXECUTABLE_OUTPUT_PATH}/pqWidgetsTest" ${test} --exit)
ENDFOREACH(test ${MyTests})

