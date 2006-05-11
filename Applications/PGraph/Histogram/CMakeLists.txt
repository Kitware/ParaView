PROJECT(PGraph_Histogram)

CMAKE_MINIMUM_REQUIRED(VERSION 2.2)

INCLUDE_DIRECTORIES(
  ${DobranoViz_SOURCE_DIR}
  ${DobranoViz_BINARY_DIR}
)

IF(VTK_USE_MPI)
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
ENDIF(VTK_USE_MPI)

# Reduce the number of dirs that get included on moc command line
# since it causes issues on Windows 2000.
GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  MainWindow.h
  ChartAdapter.h
  ${PYTHON_MOC_SOURCES}
)

SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

QT4_WRAP_UI(UI_BUILT_SOURCES
)

SOURCE_GROUP("Generated" FILES
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES}
)

IF(WIN32)
  LINK_DIRECTORIES(${QT_LIBRARY_DIR})
ENDIF(WIN32)

ADD_EXECUTABLE(PGraphHistogram WIN32 MACOSX_BUNDLE
  ChartAdapter.cxx
  ChartAdapter.h
  MainWindow.cxx
  MainWindow.h
  main.cxx
  ProcessModuleGUIHelper.cxx
  ProcessModuleGUIHelper.h
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES}
)

TARGET_LINK_LIBRARIES(PGraphHistogram
  pqWidgets
  QtChart
  QtTesting
  QtWidgets
  ${QT_XML_LIBRARIES}
)

IF(WIN32)
  TARGET_LINK_LIBRARIES(PGraphHistogram
    optimized qtmain debug qtmaind
  )
ENDIF(WIN32)

