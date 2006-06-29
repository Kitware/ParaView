PROJECT(pqClient)

SET(QT_USE_QTASSISTANT TRUE)
SET(QT_USE_QTNETWORK TRUE)
INCLUDE(${QT_USE_FILE})

IF(VTK_USE_MPI)
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
ENDIF(VTK_USE_MPI)

INCLUDE_DIRECTORIES(
  ${pqClient_SOURCE_DIR}
  ${pqClient_BINARY_DIR}
)

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

ADD_EXECUTABLE(paraview_alpha WIN32 MACOSX_BUNDLE
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

TARGET_LINK_LIBRARIES(paraview_alpha
  pqWidgets
  QtWidgets
  QtChart
  QtTesting
  ${QT_LIBRARIES}
)

IF(WIN32)
  TARGET_LINK_LIBRARIES(paraview_alpha
    optimized qtmain debug qtmaind
  )
ENDIF(WIN32)
  
IF(BUILD_TESTING)
IF(APPLE)
  SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/paraview_alpha.app/Contents/MacOS/paraview_alpha --test-directory=${PARAVIEW_TEST_DIR})
ELSE(APPLE)
  SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/paraview_alpha --test-directory=${PARAVIEW_TEST_DIR})
ENDIF(APPLE)
  SET(TEST_XML ${pqClient_SOURCE_DIR}/Testing/XML)
  SET(TEST_IMAGE ${PARAVIEW_DATA_ROOT}/Baseline)

  SET (TESTS_WITH_BASELINES
    CreateSource
    CompoundProxy
    UndoRedo
    SubMenu
    ObjectInspectorSphere
    VariableSelector
    Clip
    ClipHidden
    Cut
    CutMulti
    Threshold
    OpenFile
    ExodusPanel
    OpenSaveData
    LoadState
    Contour
    StreamTracer
    )

  SET (TESTS_WITHOUT_BASELINES
    AboutDialog
    )

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

