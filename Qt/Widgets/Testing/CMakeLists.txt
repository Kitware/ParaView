
PROJECT(QtWidgetsTesting)

INCLUDE_DIRECTORIES(${QtTesting_SOURCE_DIR})
INCLUDE_DIRECTORIES(${QtWidgetsTesting_BINARY_DIR})

# test multi view widgets
QT4_WRAP_UI(MULTIVIEW_TEST_UIS 
            ../pqMultiViewFrameMenu.ui
            )
QT4_WRAP_CPP(MULTIVIEW_TEST_MOCS 
             ../pqMultiView.h 
             ../pqMultiViewFrame.h 
             ../pqMultiViewManager.h
             )
SET(MULTIVIEW_TEST_SRCS 
    ../pqMultiView.cxx 
    ../pqMultiViewManager.cxx 
    ../pqMultiViewFrame.cxx
    )

QT4_ADD_RESOURCES(MULTIVIEW_TEST_RESOURCES
  pqMultiViewTest.qrc
)

ADD_EXECUTABLE(QtWidgetsMultiViewTest MACOSX_BUNDLE pqMultiViewTest.cxx 
               ${MULTIVIEW_TEST_SRCS} ${MULTIVIEW_TEST_RESOURCES} 
               ${MULTIVIEW_TEST_UIS} ${MULTIVIEW_TEST_MOCS}
               )
TARGET_LINK_LIBRARIES(QtWidgetsMultiViewTest QtTesting ${QT_LIBRARIES})

ADD_TEST(QtWidgetsMultiView  ${EXECUTABLE_OUTPUT_PATH}/QtWidgetsMultiViewTest 
         --run-test ${QtWidgetsTesting_SOURCE_DIR}/pqMultiViewTest.xml 
         --exit)

