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


################################################################################
SET(ServersServerManager_SRCS
  ParaViewCoreServerManagerPrintSelf
  TestComparativeAnimationCueProxy 
  TestXMLSaveLoadState
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
