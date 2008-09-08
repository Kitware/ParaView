
INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

SET(PV_TEST_INIT_COMMAND "" CACHE STRING
  "Command to run before a test begins. Multiple commands are separated by ';'.")
MARK_AS_ADVANCED(PV_TEST_INIT_COMMAND)

SET(PV_TEST_CLEAN_COMMAND "" CACHE STRING
  "Command to run after a failed test to cleanup processes.  Example: \"killall -9 rsh paraview\"")
MARK_AS_ADVANCED(PV_TEST_CLEAN_COMMAND)

SET(PV_TEST_SERVER_ROOT "localhost" CACHE STRING
  "Node which serves as the root node for the cluster when testing in parallel")
MARK_AS_ADVANCED(PV_TEST_SERVER_ROOT)

SET (PV_TEST_CLIENT "localhost" CACHE STRING
  "Node which serves as the client node, used to connect from the server side in reverse connection mode.")
MARK_AS_ADVANCED(PV_TEST_CLIENT)


SET(ConnectionXMLs
  ManualConnectCS
  ManualConnectCS-rc
  ManualConnectBuiltin
  ManualConnectCRSDS
  ManualConnectCRSDS-rc
  ManualConnectCSRR
  ManualConnectCRSDSRR
  )

FOREACH(xml ${ConnectionXMLs})
  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/${xml}.xml.in
    ${CMAKE_CURRENT_BINARY_DIR}/${xml}.xml
    @ONLY IMMEDIATE ESCAPE_QUOTES)
ENDFOREACH(xml)

SET(CONNECT_TO_SERVER_SCRIPT
  ${CMAKE_CURRENT_BINARY_DIR}/ManualConnectCS.xml)

SET(CONNECT_TO_SERVER_REVERSE_SCRIPT
  ${CMAKE_CURRENT_BINARY_DIR}/ManualConnectCS-rc.xml)

SET(CONNECT_TO_BUILTIN_SCRIPT
  ${CMAKE_CURRENT_BINARY_DIR}/ManualConnectBuiltin.xml)

SET(CONNECT_TO_RS_DS_SCRIPT
  ${CMAKE_CURRENT_BINARY_DIR}/ManualConnectCRSDS.xml)

SET(CONNECT_TO_RS_DS_REVERSE_SCRIPT
  ${CMAKE_CURRENT_BINARY_DIR}/ManualConnectCRSDS-rc.xml)

# client / server with remote rendering
SET(CONNECT_TO_SERVER_REMOTE_RENDER_SCRIPT
  ${CMAKE_CURRENT_BINARY_DIR}/ManualConnectCSRR.xml)

# client / data server / render server with remote rendering
SET(CONNECT_TO_RS_DS_REMOTE_RENDER_SCRIPT
  ${CMAKE_CURRENT_BINARY_DIR}/ManualConnectCRSDSRR.xml)

SET (PARAVIEW_CLIENT paraview)
SET (PARAVIEW_DATA_SERVER pvdataserver)
SET (PARAVIEW_SERVER pvserver)
SET (PARAVIEW_RENDER_SERVER pvrenderserver)
IF (APPLE)
  SET (PARAVIEW_CLIENT paraview.app/Contents/MacOS/paraview)
ENDIF (APPLE)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/pvTestDriverConfig.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/pvTestDriverConfig.h @ONLY IMMEDIATE
  ESCAPE_QUOTES)

ADD_EXECUTABLE(pvTestDriver${PV_EXE_SUFFIX} pvTestDriver.cxx)
ADD_DEPENDENCIES(pvTestDriver${PV_EXE_SUFFIX} paraview${PV_EXE_SUFFIX})
TARGET_LINK_LIBRARIES(pvTestDriver${PV_EXE_SUFFIX} vtksys)

INSTALL(TARGETS pvTestDriver${PV_EXE_SUFFIX}
  DESTINATION ${PV_EXE_INSTALL_CM24}
  COMPONENT Runtime)
