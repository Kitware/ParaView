PROJECT(pqClient)

IF(PARAQ_EMBED_PYTHON)
  FIND_PACKAGE(PythonLibs)

  SET(PYTHON_INCLUDE_DIRECTORIES
    ${QtPython_BINARY_DIR}
    ${QtPython_SOURCE_DIR}
  )
  SET(PYTHON_UI_FORMS
    PythonDialog.ui
  )
  SET(PYTHON_MOC_SOURCES
    PythonDialog.h
  )
  SET(PYTHON_SOURCES
    PythonDialog.cxx
    PythonDialog.h
  )
  SET(PYTHON_LINK_LIBRARIES
    pqPython
    QtPython
    vtkPVServerCommonPythonD
    vtkPVServerManagerPythonD
    ${PYTHON_LIBRARIES}
  )
ENDIF(PARAQ_EMBED_PYTHON)

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
  ${PYTHON_MOC_SOURCES}
)

SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

SET(UI_FORMS
  AboutDialog.ui
  ${PYTHON_UI_FORMS}
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

SOURCE_GROUP("Resources" FILES
  ${UI_RESOURCES}
  ${UI_FORMS}
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
  ${PYTHON_SOURCES}
  ${MOC_BUILT_SOURCES}
  ${RCS_SOURCES}
  ${UI_BUILT_SOURCES}
)

TARGET_LINK_LIBRARIES(pqClient
  pqWidgets
  QtWidgets
  QtChart
  QtTesting
  ${PYTHON_LINK_LIBRARIES}
)

IF(WIN32)
  TARGET_LINK_LIBRARIES(pqClient
    optimized qtmain debug qtmaind
  )
ENDIF(WIN32)

IF(BUILD_TESTING)
  SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/pqClient --test-directory ${PARAQ_TEST_DIR})
  SET(TEST_XML ${pqClient_SOURCE_DIR}/Testing/XML)
  SET(TEST_IMAGE ${PARAQ_DATA_ROOT}/Baseline)

  ADD_TEST(pqClientStartup        ${TEST_BINARY} --exit)
  ADD_TEST(pqClientAboutDialog    ${TEST_BINARY} --run-test ${TEST_XML}/AboutDialog.xml --exit)
  ADD_TEST(pqClientFileDialog     ${TEST_BINARY} --run-test ${TEST_XML}/FileDialog.xml --exit)
  ADD_TEST(pqClientCreateSource   ${TEST_BINARY} --run-test ${TEST_XML}/CreateSource.xml --compare-view ${TEST_IMAGE}/CreateSource.png)
  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/Testing/XML/filters.xml.in
                 ${CMAKE_CURRENT_BINARY_DIR}/Testing/XML/filters.xml COPYONLY)
  ADD_TEST(pqClientCompoundProxy  ${TEST_BINARY} --run-test ${TEST_XML}/CompoundProxy.xml --exit)
ENDIF(BUILD_TESTING)

