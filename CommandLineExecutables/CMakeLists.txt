PROJECT(PVCommmandLineExecutables)

INCLUDE_DIRECTORIES(
  ${ParaView_BINARY_DIR}
  ${PVCommon_SOURCE_DIR}
  ${PVClientServerCore_SOURCE_DIR}
  ${PVServerImplementation_SOURCE_DIR}
  ${PVServerManager_SOURCE_DIR}
  ${VTK_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )

SET(LIBS vtkPVServerManager)
IF(PARAVIEW_USE_MPI)
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
ENDIF(PARAVIEW_USE_MPI)

# Add main server executables.
SET(PV_EXE_LIST
   pvserver pvrenderserver pvdataserver)

FOREACH(pvexe ${PV_EXE_LIST})
  add_executable_with_forwarding(pv_exe_suffix
    ${pvexe} ${pvexe}.cxx)
  TARGET_LINK_LIBRARIES(${pvexe}${pv_exe_suffix} ${LIBS})

  # link enabled plugins if not building in shared library mode
  if(NOT BUILD_SHARED_LIBS)
    target_link_libraries(${pvexe}${pv_exe_suffix} ${PARAVIEW_PLUGINLIST})
  endif(NOT BUILD_SHARED_LIBS)

  # Install exe
  IF(NOT PV_INSTALL_NO_RUNTIME)
    INSTALL(TARGETS ${pvexe}
            DESTINATION ${PV_INSTALL_BIN_DIR}
            COMPONENT Runtime)
    IF (pv_exe_suffix)
      # Shared forwarding enabled.
      INSTALL(TARGETS ${pvexe}${pv_exe_suffix}
              DESTINATION ${PV_INSTALL_LIB_DIR}
              COMPONENT Runtime)
    ENDIF (pv_exe_suffix)
  ENDIF(NOT PV_INSTALL_NO_RUNTIME)

ENDFOREACH(pvexe)

SET(PV_TEST_USE_RANDOM_PORTS ON CACHE BOOL
  "Use random port numbers when testing client-server configurations.")

SET(PV_TEST_INIT_COMMAND "" CACHE STRING
  "Command to run before a test begins. Multiple commands are separated by ';'.")
MARK_AS_ADVANCED(PV_TEST_INIT_COMMAND)

SET(PV_TEST_CLEAN_COMMAND "" CACHE STRING
  "Command to run after a failed test to cleanup processes.  Example: \"killall -9 rsh paraview\"")
MARK_AS_ADVANCED(PV_TEST_CLEAN_COMMAND)

SET (PV_TEST_CLIENT "localhost" CACHE STRING
  "Node which serves as the client node, used to connect from the server side in reverse connection mode.")
MARK_AS_ADVANCED(PV_TEST_CLIENT)

# Add the test driver executable. This is not done in the above loop to avoid
# adding dependencies ${LIBS} since we only need vtksys for this executable.
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/vtkSMTestDriverConfig.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkSMTestDriverConfig.h @ONLY ESCAPE_QUOTES)

add_executable_with_forwarding(pv_exe_suffix smTestDriver vtkSMTestDriver.cxx)
TARGET_LINK_LIBRARIES(smTestDriver${pv_exe_suffix} vtksys)

#------------------------------------------------------------------------
# Install ffmpeg, if used.
IF (VTK_USE_FFMPEG_ENCODER)
  FOREACH(fmpglib avcodec avformat avutil)
    IF (FFMPEG_${fmpglib}_LIBRARY)
      pv_install_library(${FFMPEG_${fmpglib}_LIBRARY}
        ${PV_INSTALL_LIB_DIR}
        BrandedRuntime)
    ENDIF(FFMPEG_${fmpglib}_LIBRARY)
  ENDFOREACH(fmpglib)
ENDIF (VTK_USE_FFMPEG_ENCODER)

IF (NOT PV_INSTALL_NO_RUNTIME)
  INSTALL(TARGETS smTestDriver
          DESTINATION ${PV_INSTALL_BIN_DIR}
          COMPONENT Runtime)
  IF (pv_exe_suffix)
    # Shared forwarding enabled.
    INSTALL(TARGETS smTestDriver${pv_exe_suffix} 
            DESTINATION ${PV_INSTALL_LIB_DIR}
            COMPONENT Runtime)
  ENDIF (pv_exe_suffix)
ENDIF (NOT PV_INSTALL_NO_RUNTIME)

#------------------------------------------------------------------------
# We setup the CPack scripts for packaging the server.
# This comes in handy when the Qt client is not built, or we want to package
# the server executables alone (pvpython, pvbatch as well, if python was
# enabled).
INCLUDE(ParaViewBrandingCPack)
# Install ParaView client
build_paraview_client_cpack_config_init(
    PACKAGE_NAME "ParaViewServers"
    ORGANIZATION "Kitware Inc."
    VERSION_MAJOR "${PARAVIEW_VERSION_MAJOR}"
    VERSION_MINOR "${PARAVIEW_VERSION_MINOR}"
    VERSION_PATCH "${PARAVIEW_VERSION_PATCH}"
    DESCRIPTION "ParaView is a scientific visualization tool"
    LICENSE_FILE "${ParaView_SOURCE_DIR}/License_v1.2.txt"
    DESCRIPTION_FILE "${ParaView_SOURCE_DIR}/License_v1.2.txt"
)

SET (CPACK_INSTALL_CMAKE_PROJECTS_DRAGNDROP
  ${CPACK_INSTALL_CMAKE_PROJECTS})
build_paraview_client_cpack_config()
