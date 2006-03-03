PROJECT(DobranoViz)

CMAKE_MINIMUM_REQUIRED(VERSION 2.2)

FIND_PACKAGE(Qt4)
INCLUDE(${QT_USE_FILE})

SET(QT_CLIENT_VERSION_MAJOR 0)
SET(QT_CLIENT_VERSION_MINOR 9)
SET(QT_CLIENT_VERSION_PATCH 0)
SET(QT_CLIENT_VERSION ${QT_CLIENT_VERSION_MAJOR}.${QT_CLIENT_VERSION_MINOR})

ADD_DEFINITIONS(-DVERSION=${QT_CLIENT_VERSION})

INCLUDE_DIRECTORIES(
  ${DobranoViz_BINARY_DIR}
  ${pqChart_BINARY_DIR}
  ${pqChart_SOURCE_DIR}
  ${QtComponents_SOURCE_DIR}
  ${QtTesting_BINARY_DIR}
  ${QtTesting_SOURCE_DIR}
  ${QtWidgets_BINARY_DIR}
  ${QtWidgets_SOURCE_DIR}
  ${PYTHON_INCLUDE_DIRECTORIES}
  ${ParaQ_SOURCE_DIR}/ParaView/Servers/Common
  ${ParaQ_SOURCE_DIR}/ParaView/Servers/Filters
  ${ParaQ_SOURCE_DIR}/ParaView/Servers/ServerManager
  ${ParaQ_SOURCE_DIR}/ParaView/Utilities/VTKClientServer
  ${ParaQ_BINARY_DIR}/ParaView/Utilities/VTKClientServer
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/Common
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/Filtering
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/Graphics
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/GUISupport/Qt
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/Hybrid
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/IO
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/Rendering
  ${ParaQ_SOURCE_DIR}/Components/GeoViz
  ${ParaQ_BINARY_DIR}/ParaView/VTK
)
IF(VTK_USE_MPI)
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
ENDIF(VTK_USE_MPI)

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  DobranoVizWindow.h
  LineChartAdapter.h
  ${PYTHON_MOC_SOURCES}
)

QT4_WRAP_UI(UI_BUILT_SOURCES
  ${UI_FORMS}
)

SOURCE_GROUP("Generated" FILES
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES}
)

IF(WIN32)
  LINK_DIRECTORIES(${QT_LIBRARY_DIR})
ENDIF(WIN32)

ADD_EXECUTABLE(DobranoViz WIN32 MACOSX_BUNDLE
  main.cxx
  DobranoVizWindow.cxx
  DobranoVizWindow.h
  ImageLinePlot.cxx
  ImageLinePlot.h
  LineChartAdapter.cxx
  LineChartAdapter.h
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES}
)

TARGET_LINK_LIBRARIES(DobranoViz
  QtComponents
  QtWidgets
  QtTesting
)

IF(WIN32)
  TARGET_LINK_LIBRARIES(DobranoViz
    optimized qtmain debug qtmaind
  )
ENDIF(WIN32)

