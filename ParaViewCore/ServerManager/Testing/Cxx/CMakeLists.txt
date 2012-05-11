INCLUDE_DIRECTORIES(
  ${ParaView_BINARY_DIR}
  ${VTKCLIENTSERVER_INCLUDE_DIR}
  ${VTK_INCLUDE_DIR}
  ${XDMF_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${protobuf_SOURCE_DIR}/src
  )

ADD_EXECUTABLE(TestSubProxy
  TestSubProxy.cxx)

TARGET_LINK_LIBRARIES(TestSubProxy
  vtkPVServerManager)

ADD_EXECUTABLE(TestMultipleSessions
  TestMultipleSessions.cxx)

TARGET_LINK_LIBRARIES(TestMultipleSessions
  vtkPVServerManager)

################################################################################
SET(ServersServerManager_SRCS
  ParaViewCoreServerManagerPrintSelf
  TestComparativeAnimationCueProxy 
  TestXMLSaveLoadState
  TestProxyAnnotation
  )

FOREACH(name ${ServersServerManager_SRCS})
  ADD_EXECUTABLE(${name} ${name}.cxx)
  ADD_TEST(${name} ${CXX_TEST_PATH}/${name} ${name}
        -D ${VTK_DATA_ROOT})
  TARGET_LINK_LIBRARIES(${name} vtkPVServerManager)
ENDFOREACH(name)

################################################################################
# Requires that PVServerManagerTestData is set
# for any of the tests to be added.
INCLUDE_DIRECTORIES(
  ${ServerManager_SOURCE_DIR}
  ${ServerManager_BINARY_DIR}
  ${VTK_INCLUDE_DIR}
  )
 
# This is the executable that can load any Server Manager state (*.pvsm) file
# and test the rendered image.
ADD_EXECUTABLE(ServerManagerStateLoader
  ServerManagerStateLoader.cxx
  vtkTestingProcessModuleGUIHelper.cxx
  vtkTestingProcessModuleGUIHelper.h
  vtkTestingOptions.cxx
  vtkTestingOptions.h)

TARGET_LINK_LIBRARIES(ServerManagerStateLoader
  vtkPVServerManager)

################################################################################
# Add the *.pvsm file names to load and test.
SET(TEST_STATES
  ${CXX_SMSTATE_FILES}
  )

IF (PVServerManagerTestData)
  FOREACH (tfile  ${TEST_STATES})
    ADD_TEST("${tfile}-ServerManager"
      ${EXECUTABLE_OUTPUT_PATH}/ServerManagerStateLoader
      -D=${PVServerManagerTestData}
      -T=${ParaView_BINARY_DIR}/Testing/Temporary
      -V=${PVServerManagerTestData}/Baseline/${tfile}.png
      ${SMSTATE_FILE_DIR}/${tfile}.pvsm)
  ENDFOREACH(tfile)
ENDIF (PVServerManagerTestData)

################################################################################
# ParaView ServerManager Unit Tests
if (PARAVIEW_ENABLE_QT_SUPPORT)
  find_package(Qt4 COMPONENTS QtCore QtTest REQUIRED)
  set(QT_DONT_USE_QTGUI TRUE)
  set(QT_USE_QTTEST TRUE)

  include(${QT_USE_FILE})
  if(${CMAKE_VERSION} VERSION_LESS 2.8.7 AND NOT WIN32)
    include(Qt4Macros-CMake2.8.7)
  endif()

  set(UNITTESTS
    vtkSMProxyTest
    vtkSMProxyLinkTest
    vtkSMDoubleVectorPropertyTest
    vtkSMIntVectorPropertyTest
    vtkSMStringVectorPropertyTest
    vtkSMUndoStackTest
    vtkSMPropertyHelperTest
  )

  foreach(name ${UNITTESTS})
    qt4_wrap_cpp(${name}_MOC_SOURCES ${name}.h)
    add_executable(${name} ${name}.cxx ${${name}_MOC_SOURCES})
    add_test(pvunit.${name} ${CXX_TEST_PATH}/${name})
    target_link_libraries(${name} vtkPVServerManager ${QT_LIBRARIES})
  endforeach()
endif()
