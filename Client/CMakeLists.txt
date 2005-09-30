
CMAKE_MINIMUM_REQUIRED(VERSION 2.2)

PROJECT(Client)

FIND_PACKAGE(Qt4)
SET(QT_USE_QT3SUPPORT TRUE)
INCLUDE(${QT_USE_FILE})


# find QtTestLib
EXEC_PROGRAM( ${QT_QMAKE_EXECUTABLE} 
              ARGS -query QT_INSTALL_PREFIX
              OUTPUT_VARIABLE qt_install_prefix )
FIND_PATH(QT_TEST_INCLUDE_DIR QtTest
      NAMES "${qt_install_prefix}/include/QtTest" 
            "${qt_install_prefix}/solutions/qttestlib/src/QtTest" 
      CACHE PATH "The top-level path to the QtTest4 headers")
FIND_LIBRARY( QT_TEST_LIBRARY NAMES QtTest QtTest2
              PATHS ${qt_install_prefix}/solutions/qttestlib/src 
                    ${qt_install_prefix}/lib)

INCLUDE_DIRECTORIES(
  ${ParaQ_BINARY_DIR}/Client
  ${ParaQ_BINARY_DIR}/ParaView
  ${ParaQ_BINARY_DIR}/ParaView/VTK
  ${ParaQ_BINARY_DIR}/ParaView/Servers/Common
  ${ParaQ_BINARY_DIR}/ParaView/Servers/Filters
  ${ParaQ_BINARY_DIR}/ParaView/Servers/ServerManager
  ${ParaQ_BINARY_DIR}/ParaView/Utilities/VTKClientServer
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/Common
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/Filtering
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/GenericFiltering
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/Rendering
  ${ParaQ_SOURCE_DIR}/ParaView/GUI/Client
  ${ParaQ_SOURCE_DIR}/ParaView/Servers/Common
  ${ParaQ_SOURCE_DIR}/ParaView/Servers/Filters
  ${ParaQ_SOURCE_DIR}/ParaView/Servers/ServerManager
  ${ParaQ_SOURCE_DIR}/ParaView/Utilities/VTKClientServer
  ${QT_TEST_INCLUDE_DIR}
)

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  pqMainWindow.h
  pqServerFileBrowser.h
  pqServerFileBrowserList.h
  pqTests.h
)

ADD_EXECUTABLE(pqClient
  main.cxx
  pqCamera.cxx
  pqCamera.h
  pqMainWindow.cxx
  pqMainWindow.h
  pqOptions.cxx
  pqOptions.h
  pqParts.cxx
  pqParts.h
  pqServer.cxx
  pqServer.h
  pqServerFileBrowser.cxx
  pqServerFileBrowser.h
  pqServerFileBrowserList.cxx
  pqServerFileBrowserList.h
  pqTests.cxx
  pqTests.h
  ${MOC_BUILT_SOURCES}
)

TARGET_LINK_LIBRARIES(pqClient
  vtkPVServerManager
  vtkPVFiltersCS
  vtkPVFilters
  vtkPVServerCommonCS
  vtkPVServerCommon
  vtkParallelCS
  vtkWidgetsCS
  vtkHybridCS
  vtkVolumeRenderingCS
  vtkXdmfCS
  vtkRenderingCS
  vtkGenericFilteringCS
  vtkGenericFiltering
  vtkIOCS
  vtkImagingCS
  vtkGraphicsCS
  vtkFilteringCS
  vtkCommonCS
  vtkClientServer
  SPCTH

  vtkParallel
  vtkHybrid
  vtkIO
  vtkVolumeRendering
  vtkWidgets
  vtkImaging
  vtkRendering
  vtkGraphics
  vtkFiltering
  vtkCommon
  vtkXdmf
  Xdmf
  vtkexoIIc
  vtkNetCDF
  vtkexpat
  vtkftgl
  vtkhdf5
  vtktiff
  vtkzlib
  vtkMPEG2Encode
  vtkDICOMParser

  ${QT_LIBRARIES}
  ${QT_TEST_LIBRARY}

  ${JPEG_LIBRARY}
  ${PNG_LIBRARY}
  ${FREETYPE_LIBRARY}
)


