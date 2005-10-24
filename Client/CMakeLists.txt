
CMAKE_MINIMUM_REQUIRED(VERSION 2.2)

PROJECT(Client)

FIND_PACKAGE(Qt4)
INCLUDE(${QT_USE_FILE})


# find QtTestLib
EXEC_PROGRAM( ${QT_QMAKE_EXECUTABLE} 
              ARGS -query QT_INSTALL_PREFIX
              OUTPUT_VARIABLE qt_install_prefix )
FIND_PATH(QT_TEST_INCLUDE_DIR QtTest
      "${qt_install_prefix}/include/QtTest" 
      "${qt_install_prefix}/solutions/qttestlib/src/QtTest" 
      "$ENV{HOME}/Software/Develop/Qt/QtTest" 
      DOC "The top-level path to the QtTest4 headers")
FIND_LIBRARY( QT_TEST_LIBRARY NAMES QtTest QtTest2
  PATHS
  "${qt_install_prefix}/solutions/qttestlib/src"
  "${qt_install_prefix}/lib"
  "$ENV{HOME}/Software/Develop/Qt/QtTest/lib" 
  )
FIND_LIBRARY( QT_TEST_DEBUG_LIBRARY NAMES QtTest_debug QtTest2_debug
  PATHS
  "${qt_install_prefix}/solutions/qttestlib/src"
  "${qt_install_prefix}/lib"
  "$ENV{HOME}/Software/Develop/Qt/QtTest/lib" 
  )

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
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/IO
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/Rendering
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/GUISupport/Qt
  ${ParaQ_SOURCE_DIR}/ParaView/GUI/Client
  ${ParaQ_SOURCE_DIR}/ParaView/Servers/Common
  ${ParaQ_SOURCE_DIR}/ParaView/Servers/Filters
  ${ParaQ_SOURCE_DIR}/ParaView/Servers/ServerManager
  ${ParaQ_SOURCE_DIR}/ParaView/Utilities/VTKClientServer
  ${QT_TEST_INCLUDE_DIR}
  ${QT_TEST_INCLUDE_DIR}/..
)

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  pqCheckBox.h
  pqFileDialog.h
  pqFileDialogModel.h
  pqLocalFileDialogModel.h
  pqMainWindow.h
  pqServerBrowser.h
  pqServerFileDialogModel.h
  pqSpinBox.h
  pqTestCases.h
)

ADD_EXECUTABLE(pqClient
  main.cxx
  pqCamera.cxx
  pqCamera.h
  pqCheckBox.cxx
  pqCheckBox.h
  pqFileDialog.cxx
  pqFileDialog.h
  pqFileDialogModel.cxx
  pqFileDialogModel.h
  pqLocalFileDialogModel.cxx
  pqLocalFileDialogModel.h
  pqMainWindow.cxx
  pqMainWindow.h
  pqOptions.cxx
  pqOptions.h
  pqParts.cxx
  pqParts.h
  pqProperties.cxx
  pqProperties.h
  pqServer.cxx
  pqServer.h
  pqServerBrowser.cxx
  pqServerBrowser.h
  pqServerFileDialogModel.cxx
  pqServerFileDialogModel.h
  pqSpinBox.cxx
  pqSpinBox.h
  pqTestCases.cxx
  pqTestCases.h
  pqTesting.cxx
  pqTesting.h
  pqRenderViewProxy.cxx
  pqRenderViewProxy.h
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

  QVTK

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
  optimized ${QT_TEST_LIBRARY} debug ${QT_TEST_DEBUG_LIBRARY}

  ${JPEG_LIBRARY}
  ${PNG_LIBRARY}
  ${FREETYPE_LIBRARY}
)


