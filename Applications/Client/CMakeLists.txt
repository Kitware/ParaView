PROJECT(pqClient)

IF(VTK_USE_MPI)
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
ENDIF(VTK_USE_MPI)

INCLUDE_DIRECTORIES(
  ${pqClient_SOURCE_DIR}
  ${pqClient_BINARY_DIR}
)

ADD_DEFINITIONS(-DVERSION=${PARAQ_VERSION})

# Reduce the number of dirs that get included on moc command line
# since it causes issues on Windows 2000.
GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  AboutDialog.h
  MainWindow.h
)

SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

SET(UI_FORMS
  AboutDialog.ui
)

QT4_WRAP_UI(UI_BUILT_SOURCES
  ${UI_FORMS}
)

SET(UI_RESOURCES
  Resources.qrc
)

QT4_ADD_RESOURCES(RCS_SOURCES
  ${UI_RESOURCES}
)

IF(WIN32)
  SET(EXE_ICON pqClient.rc)
ENDIF(WIN32)

SOURCE_GROUP("Resources" FILES
  ${UI_RESOURCES}
  ${UI_FORMS}
  ${EXE_ICON}
)

SOURCE_GROUP("Generated" FILES
  ${MOC_BUILT_SOURCES}
  ${RCS_SOURCES}
  ${UI_BUILT_SOURCES}
)

IF(WIN32)
  LINK_DIRECTORIES(${QT_LIBRARY_DIR})
ENDIF(WIN32)

ADD_EXECUTABLE(pqClient WIN32 MACOSX_BUNDLE
  AboutDialog.cxx
  AboutDialog.h
  MainWindow.cxx
  MainWindow.h
  main.cxx
  ProcessModuleGUIHelper.cxx
  ProcessModuleGUIHelper.h
  ${MOC_BUILT_SOURCES}
  ${RCS_SOURCES}
  ${UI_BUILT_SOURCES}
  ${EXE_ICON}
)

TARGET_LINK_LIBRARIES(pqClient
  pqWidgets
  QtWidgets
  QtChart
  QtTesting
)

IF(WIN32)
  TARGET_LINK_LIBRARIES(pqClient
    optimized qtmain debug qtmaind
  )
ENDIF(WIN32)

IF(BUILD_TESTING)
  SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/pqClient --test-directory=${PARAQ_TEST_DIR})
  SET(TEST_XML ${pqClient_SOURCE_DIR}/Testing/XML)
  SET(TEST_IMAGE ${PARAQ_DATA_ROOT}/Baseline)

  SET (TESTS_WITH_BASELINES
    CreateSource
    CompoundProxy
    UndoRedo
    SubMenu
    ObjectInspectorSphere
    ObjectInspectorGlyph
    VariableSelector
    )

  SET (TESTS_WITHOUT_BASELINES
    AboutDialog
    FileDialog
    )

  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/Testing/XML/filters.xml.in
                 ${CMAKE_CURRENT_BINARY_DIR}/Testing/XML/filters.xml COPYONLY)

  # Special-case test to see that Qt widgets are all named properly for test playback
  ADD_TEST(pqClientWidgetNaming  ${TEST_BINARY} --test-ui-names --exit)
  ADD_TEST(pqClientWidgetNaming2  ${TEST_BINARY} --run-test=${TEST_XML}/WidgetNaming2.xml --test-ui-names --exit)
               
  FOREACH(test ${TESTS_WITHOUT_BASELINES})
    ADD_TEST(pqClient${test}
      ${TEST_BINARY} --run-test=${TEST_XML}/${test}.xml --exit)
  ENDFOREACH(test)
   
  FOREACH(test ${TESTS_WITH_BASELINES})
    ADD_TEST(pqClient${test}
      ${TEST_BINARY} --run-test=${TEST_XML}/${test}.xml
      --compare-view=${TEST_IMAGE}/${test}.png
      --exit)
  ENDFOREACH(test) 
  
ENDIF(BUILD_TESTING)

