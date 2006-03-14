PROJECT(QtClient)

CMAKE_MINIMUM_REQUIRED(VERSION 2.2)

FIND_PACKAGE(Qt4)
INCLUDE(${QT_USE_FILE})

SET(QT_CLIENT_VERSION_MAJOR 0)
SET(QT_CLIENT_VERSION_MINOR 9)
SET(QT_CLIENT_VERSION_PATCH 0)
SET(QT_CLIENT_VERSION ${QT_CLIENT_VERSION_MAJOR}.${QT_CLIENT_VERSION_MINOR})

ADD_DEFINITIONS(-DVERSION=${QT_CLIENT_VERSION})

IF(PARAQ_EMBED_PYTHON)
  FIND_PACKAGE(PythonLibs)

  SET(PYTHON_INCLUDE_DIRECTORIES
    ${QtPython_BINARY_DIR}
    ${QtPython_SOURCE_DIR}
  )
  SET(PYTHON_UI_FORMS
    pqPython.ui
  )
  SET(PYTHON_MOC_SOURCES
    pqPythonDialog.h
  )
  SET(PYTHON_SOURCES
    pqPythonDialog.cxx
    pqPythonDialog.h
  )
  SET(PYTHON_LINK_LIBRARIES
    pqPython
    QtPython
    vtkPVServerCommonPythonD
    vtkPVServerManagerPythonD
    ${PYTHON_LIBRARIES}
  )
ENDIF(PARAQ_EMBED_PYTHON)

INCLUDE_DIRECTORIES(
  ${QtClient_BINARY_DIR}
  ${pqChart_BINARY_DIR}
  ${pqChart_SOURCE_DIR}
  ${QtTesting_BINARY_DIR}
  ${QtTesting_SOURCE_DIR}
  ${QtWidgets_BINARY_DIR}
  ${QtWidgets_SOURCE_DIR}
  ${QtComponents_SOURCE_DIR}
  ${PYTHON_INCLUDE_DIRECTORIES}
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/Graphics
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/Hybrid
)
IF(VTK_USE_MPI)
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
ENDIF(VTK_USE_MPI)

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  pqAboutDialog.h
  pqMainWindow.h
  ${PYTHON_MOC_SOURCES}
)

SET(UI_FORMS
  pqAbout.ui
  ${PYTHON_UI_FORMS}
)

QT4_WRAP_UI(UI_BUILT_SOURCES
  ${UI_FORMS}
)

SET(UI_RESOURCES
  pqClient.qrc
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
  main.cxx
  pqAboutDialog.cxx
  pqAboutDialog.h
  pqMainWindow.cxx
  pqMainWindow.h
  ${PYTHON_SOURCES}
  ${MOC_BUILT_SOURCES}
  ${RCS_SOURCES}
  ${UI_BUILT_SOURCES}
)

TARGET_LINK_LIBRARIES(pqClient
  QtWidgets
  QtComponents
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
  SET(TEST_XML ${QtClient_SOURCE_DIR}/Testing/XML)
  SET(TEST_IMAGE ${PARAQ_DATA_ROOT}/Baseline)

  ADD_TEST(QtClientStartup        ${TEST_BINARY} --exit)
  ADD_TEST(QtClientAboutDialog    ${TEST_BINARY} --run-test ${TEST_XML}/AboutDialog.xml --exit)
  ADD_TEST(QtClientFileDialog     ${TEST_BINARY} --run-test ${TEST_XML}/FileDialog.xml --exit)
  ADD_TEST(QtClientCreateSource   ${TEST_BINARY} --run-test ${TEST_XML}/CreateSource.xml --compare-view ${TEST_IMAGE}/CreateSource.png)
  CONFIGURE_FILE(${QtClient_SOURCE_DIR}/Testing/XML/filters.xml
                 ${QtClient_BINARY_DIR}/Testing/XML/filters.xml COPYONLY)
  ADD_TEST(QtClientCompoundProxy  ${TEST_BINARY} --run-test ${TEST_XML}/CompoundProxy.xml --exit)
ENDIF(BUILD_TESTING)

CONFIGURE_FILE(${QtClient_SOURCE_DIR}/pqConfig.h.in
               ${QtClient_BINARY_DIR}/pqConfig.h)

