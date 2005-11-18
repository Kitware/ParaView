
PROJECT(QtWidgetsTesting)

INCLUDE_DIRECTORIES(${QtTesting_SOURCE_DIR})

# test multi view widgets
SET(TEST_DIR ${QtWidgets_SOURCE_DIR}/Testing)
SET(WIDGETS_DIR ${QtWidgets_SOURCE_DIR}/Testing)
QT4_WRAP_UI(TEST_MV_UIS 
            ../pqMultiViewFrameMenu.ui
            )
QT4_WRAP_CPP(TEST_MV_MOCS 
             ../pqMultiView.h 
             ../pqMultiViewFrame.h 
             ../pqMultiViewManager.h
             )
SET(MULTIVIEW_TEST_SRCS 
    ../pqMultiView.cxx 
    ../pqMultiViewManager.cxx 
    ../pqMultiViewFrame.cxx
    )

QT4_ADD_RESOURCES(RCS_SOURCES
  pqMultiViewTest.qrc
)

ADD_EXECUTABLE(QtWidgetsMultiViewTest pqMultiViewTest.cxx 
               ${MULTIVIEW_TEST_SRCS} ${RCS_SOURCES} ${TEST_MV_UIS} ${TEST_MV_MOCS})
TARGET_LINK_LIBRARIES(QtWidgetsMultiViewTest QtTesting ${QT_LIBRARIES})
ADD_TEST(QtWidgetsMultiView  ${EXECUTABLE_OUTPUT_PATH}/QtWidgetsMultiViewTest --run-test ${TEST_DIR}/pqMultiViewTest.xml --exit)

