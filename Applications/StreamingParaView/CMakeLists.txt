PROJECT(StreamingParaView)

INCLUDE(${ParaView_CMAKE_DIR}/ParaViewBranding.cmake)
INCLUDE(${ParaView_CMAKE_DIR}/ParaViewPlugins.cmake)

add_subdirectory(Plugin)

QT4_WRAP_CPP(MOC_BUILT_SOURCES
#  pqCustomViewModules.h
  pqCustomDisplayPolicy.h
  streamingMainWindow.h
)

QT4_WRAP_UI(UI_BUILT_SOURCES
  streamingMainWindow.ui
)

SET(Application_SOURCE_FILES
  streamingMainWindow.cxx
  streamingMainWindow.h
#  pqCustomViewModules.cxx
  pqCustomDisplayPolicy.cxx
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES}
)

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

build_paraview_client(streaming_paraview
  TITLE "ParaView (Streaming)"
  ORGANIZATION  "Kitware Inc."
  VERSION_MAJOR 1 
  VERSION_MINOR 1
  VERSION_PATCH 1
#  SPLASH_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/RSplash.png"
  PVMAIN_WINDOW streamingMainWindow
  PVMAIN_WINDOW_INCLUDE streamingMainWindow.h
  REQUIRED_PLUGINS StreamingPlugin
  GUI_CONFIGURATION_XMLS
    ${CMAKE_CURRENT_SOURCE_DIR}/CustomResources/CustomWriters.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/CustomResources/CustomReaders.xml 
    ${CMAKE_CURRENT_SOURCE_DIR}/CustomResources/CustomFilters.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/CustomResources/CustomSources.xml
  SOURCES ${Application_SOURCE_FILES}
  INSTALL_BIN_DIR "${PV_INSTALL_BIN_DIR}"
  INSTALL_LIB_DIR "${PV_INSTALL_LIB_DIR}"
)

IF(FALSE)#BUILD_TESTING)
  #PART1 - Tests that run tests inside the application
  SET(TEST_XML ${StreamingParaView_SOURCE_DIR}/Testing/XML)
  SET(TEST_PYTHON ${StreamingParaView_SOURCE_DIR}/Testing/Python)
  SET(TEST_IMAGE ${PARAVIEW_DATA_ROOT}/Baseline)

  #StreamingApplication depends upon the streaming plugin
  SET(DOLLAR "$")
  IF(CMAKE_CONFIGURATION_TYPES)
    SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/${DOLLAR}{CTEST_CONFIGURATION_TYPE}/${APP_NAME} --test-directory=${PARAVIEW_TEST_DIR})
    SET(RUN_PATH ${EXECUTABLE_OUTPUT_PATH}/${DOLLAR}{CTEST_CONFIGURATION_TYPE})
  ELSE(CMAKE_CONFIGURATION_TYPES)
    SET(RUN_PATH ${EXECUTABLE_OUTPUT_PATH})
    IF(Q_WS_MAC)
      SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/${APP_NAME}.app/Contents/MacOS/${APP_NAME} --test-directory=${PARAVIEW_TEST_DIR})
    ELSE(Q_WS_MAC)
      SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/${APP_NAME} --test-directory=${PARAVIEW_TEST_DIR})
    ENDIF(Q_WS_MAC)
  ENDIF(CMAKE_CONFIGURATION_TYPES)

  IF (QT_TESTING_WITH_PYTHON AND PARAVIEW_DATA_ROOT)
    SET(PLUGIN_TESTS
      TestStreamingApplication
      )
    
    FOREACH(test ${PLUGIN_TESTS})
      ADD_TEST(pqSClient${test}
        ${CMAKE_COMMAND} -E chdir ${RUN_PATH}
        ${TEST_BINARY} -dr --run-test=${TEST_PYTHON}/${test}.py
        --test-directory=${PARAVIEW_TEST_DIR}
        --compare-view=${TEST_IMAGE}/${test}.png
        ${THRESHOLD_CMD}
        --exit)
    ENDFOREACH(test)
  ENDIF (QT_TESTING_WITH_PYTHON AND PARAVIEW_DATA_ROOT)

  #PART2 - Tests that use a driver to orchestrate several executables (client/server etc)

ENDIF(FALSE)#BUILD_TESTING)
  
