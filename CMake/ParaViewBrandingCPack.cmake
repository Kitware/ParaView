###############################################################################
# This file defines the macro for cpack configuration generation for a
# ParaView-based client builds with custom branding and configuration.
#
# build_paraview_client_cpack_config_init(
#   PACKAGE_NAME "ParaView"
#   ORGANIZATION "Kitware Inc."
#   VERSION_MAJOR 0
#   VERSION_MINOR 0
#   VERSION_PATCH 0
#   DESCRIPTION "Short Description"
#   LICENSE_FILE "License.txt"
#   DESCRIPTION_FILE "Description.txt"
#   PACKAGE_EXECUTABLES "paraview;ParaView"
#   )
#
# After build_paraview_client_cpack_config_init() the application is free to
# modify cpack variables as deemed necessary before calling
# build_paraview_client_cpack_config() which will generate the config.
#
# build_paraview_client_cpack_config()
#
message(WARNING "ParaViewBrandingCPack.cmake is deprecated. "
  "Applications are expected to manage their own cpack rules by "
  "including CPack.cmake from CMake directly")

MACRO(build_paraview_client_cpack_config_init)
  PV_PARSE_ARGUMENTS("BCC"
    "PACKAGE_NAME;ORGANIZATION;VERSION_MAJOR;VERSION_MINOR;VERSION_PATCH;DESCRIPTION;LICENSE_FILE;DESCRIPTION_FILE;PACKAGE_EXECUTABLES;"
    ""
    ${ARGN}
    )

  IF(APPLE)
    SET(CPACK_BINARY_TBZ2 OFF)
    SET(CPACK_BINARY_DRAGNDROP ON)
    SET(CPACK_BINARY_PACKAGEMAKER OFF)
    SET(CPACK_BINARY_STGZ OFF)
  ENDIF()

  SET(CPACK_PACKAGE_NAME "${BCC_PACKAGE_NAME}")
  SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${BCC_DESCRIPTION}")
  SET(CPACK_PACKAGE_VENDOR "${BCC_ORGANIZATION}")
  IF (BCC_DESCRIPTION_FILE)
    SET(CPACK_PACKAGE_DESCRIPTION_FILE "${BCC_DESCRIPTION_FILE}")
  ENDIF()
  IF (BCC_LICENSE_FILE)
    SET(CPACK_RESOURCE_FILE_LICENSE "${BCC_LICENSE_FILE}")
  ENDIF()
  SET(CPACK_PACKAGE_VERSION_MAJOR "${BCC_VERSION_MAJOR}")
  SET(CPACK_PACKAGE_VERSION_MINOR "${BCC_VERSION_MINOR}")
  SET(CPACK_PACKAGE_VERSION_PATCH "${BCC_VERSION_PATCH}")
  SET(CPACK_PACKAGE_EXECUTABLES "${BCC_PACKAGE_EXECUTABLES}")
  SET(CPACK_PACKAGE_INSTALL_DIRECTORY "${BCC_PACKAGE_NAME} ${BCC_VERSION_MAJOR}.${BCC_VERSION_MINOR}.${BCC_VERSION_PATCH}")
  SET(CPACK_NSIS_MODIFY_PATH OFF)
  SET(CPACK_STRIP_FILES OFF)
  SET(CPACK_SOURCE_STRIP_FILES OFF)
  SET (CPACK_OUTPUT_CONFIG_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/CPack${BCC_PACKAGE_NAME}Config.cmake")

  IF (CMAKE_SYSTEM_PROCESSOR MATCHES "unknown")
    SET (CMAKE_SYSTEM_PROCESSOR "x86")
  ENDIF ()
  IF(NOT DEFINED CPACK_SYSTEM_NAME)
    SET(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
  ENDIF()
  IF(${CPACK_SYSTEM_NAME} MATCHES Windows)
    IF(CMAKE_CL_64)
      SET(CPACK_SYSTEM_NAME Win64-${CMAKE_SYSTEM_PROCESSOR})
    ELSE()
      SET(CPACK_SYSTEM_NAME Win32-${CMAKE_SYSTEM_PROCESSOR})
    ENDIF()
  ENDIF()

  IF(${CPACK_SYSTEM_NAME} MATCHES Darwin AND CMAKE_OSX_ARCHITECTURES)
    list(LENGTH CMAKE_OSX_ARCHITECTURES _length)
    IF(_length GREATER 1)
      SET(CPACK_SYSTEM_NAME Darwin-Universal)
    ELSE()
      SET(CPACK_SYSTEM_NAME Darwin-${CMAKE_OSX_ARCHITECTURES})
    ENDIF()
  ENDIF()


  SET (CPACK_INSTALL_CMAKE_PROJECTS 
    "${ParaView_BINARY_DIR}" "ParaView Runtime Libs" "Runtime" "/"
    "${ParaView_BINARY_DIR}" "VTK Runtime Libs" "RuntimeLibraries" "/"
    "${ParaView_BINARY_DIR}" "HDF5 Core Library" "libraries" "/"
  )

  IF(HDF5_BUILD_CPP_LIB)
    LIST(APPEND CPACK_INSTALL_CMAKE_PROJECTS
      "${ParaView_BINARY_DIR}" "HDF5 C++ Library" "cpplibraries" "/"
    )
  ENDIF()

  IF(HDF5_BUILD_HL_LIB)
    LIST(APPEND CPACK_INSTALL_CMAKE_PROJECTS
      "${ParaView_BINARY_DIR}" "HDF5 HL Library" "hllibraries" "/"
    )
    IF(HDF5_BUILD_CPP_LIB)
      LIST(APPEND CPACK_INSTALL_CMAKE_PROJECTS
        "${ParaView_BINARY_DIR}" "HDF5 HL C++ Library" "hlcpplibraries" "/"
      )
    ENDIF()
  ENDIF()

  # Append in CPACK rule for the Development Component
  IF(NOT PV_INSTALL_NO_DEVELOPMENT)
    LIST(APPEND CPACK_INSTALL_CMAKE_PROJECTS
      "${ParaView_BINARY_DIR}" "ParaView Development Headers, Libs and Tools" "Development" "/"
      "${ParaView_BINARY_DIR}" "HDF5 Core Headers" "headers" "/"
    )

    IF(HDF5_BUILD_CPP_LIB)
      LIST(APPEND CPACK_INSTALL_CMAKE_PROJECTS
        "${ParaView_BINARY_DIR}" "HDF5 C++ Library" "cppheaders" "/"
      )
    ENDIF()

    IF(HDF5_BUILD_HL_LIB)
      LIST(APPEND CPACK_INSTALL_CMAKE_PROJECTS
        "${ParaView_BINARY_DIR}" "HDF5 HL Library" "hllheaders" "/"
      )

      IF(HDF5_BUILD_CPP_LIB)
        LIST(APPEND CPACK_INSTALL_CMAKE_PROJECTS
          "${ParaView_BINARY_DIR}" "HDF5 HL C++ Library" "hlcppheaders" "/"
        )
      ENDIF()
    ENDIF()
  ENDIF()

  LIST(APPEND CPACK_INSTALL_CMAKE_PROJECTS
    "${CMAKE_CURRENT_BINARY_DIR}" "${BCC_PACKAGE_NAME} Components" "BrandedRuntime" "/"
  )

  # Override this variable to choose a different component for mac drag-n-drop
  # generator.
  SET (CPACK_INSTALL_CMAKE_PROJECTS_DRAGNDROP
    ${CPACK_INSTALL_CMAKE_PROJECTS})
ENDMACRO()

MACRO(build_paraview_client_cpack_config)
  CONFIGURE_FILE("${ParaView_CMAKE_DIR}/ParaViewCPackOptions.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CPack${CPACK_PACKAGE_NAME}Options.cmake"
    @ONLY
    )
  SET (CPACK_PROJECT_CONFIG_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/CPack${CPACK_PACKAGE_NAME}Options.cmake")
  INCLUDE(CPack)
ENDMACRO()


# Function to install qt libraries. qtliblist is a list of libraries to install
# of the form "QTCORE QTGUI QTNETWORK QTXML QTTEST QTSQL" etc.
FUNCTION(install_qt_libs qtliblist componentname)
  IF (NOT APPLE)
    FOREACH(qtlib ${qtliblist})
      IF (QT_${qtlib}_LIBRARY_RELEASE)
        IF (NOT WIN32)
          #GET_FILENAME_COMPONENT(QT_LIB_DIR_tmp ${QT_${qtlib}_LIBRARY_RELEASE} PATH)
          #GET_FILENAME_COMPONENT(QT_LIB_NAME_tmp ${QT_${qtlib}_LIBRARY_RELEASE} NAME)
          #FILE(GLOB QT_LIB_LIST RELATIVE "${QT_LIB_DIR_tmp}" "${QT_${qtlib}_LIBRARY_RELEASE}*")
          #IF(NOT ${QT_LIB_NAME_tmp} MATCHES "\\.debug$")
          #  INSTALL(CODE "
          #    MESSAGE(STATUS \"!!!!Installing \${CMAKE_INSTALL_PREFIX}/${PV_INSTALL_LIB_DIR}/${QT_LIB_NAME_tmp}\")
          #    EXECUTE_PROCESS (WORKING_DIRECTORY ${QT_LIB_DIR_tmp}
          #         COMMAND tar c ${QT_LIB_LIST}
          #         COMMAND tar -xC \${CMAKE_INSTALL_PREFIX}/${PV_INSTALL_LIB_DIR})
          #         " COMPONENT ${componentname})
          # ENDIF()
          # Install .so and versioned .so.x.y
          GET_FILENAME_COMPONENT(QT_LIB_DIR_tmp ${QT_${qtlib}_LIBRARY_RELEASE} PATH)
          GET_FILENAME_COMPONENT(QT_LIB_NAME_tmp ${QT_${qtlib}_LIBRARY_RELEASE} NAME)
          INSTALL(DIRECTORY ${QT_LIB_DIR_tmp}/ DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
                FILES_MATCHING PATTERN "${QT_LIB_NAME_tmp}*"
                PATTERN "${QT_LIB_NAME_tmp}*.debug" EXCLUDE)
        ELSE ()
          GET_FILENAME_COMPONENT(QT_DLL_PATH_tmp ${QT_QMAKE_EXECUTABLE} PATH)
          GET_FILENAME_COMPONENT(QT_LIB_NAME_tmp ${QT_${qtlib}_LIBRARY_RELEASE} NAME_WE)
          INSTALL(FILES ${QT_DLL_PATH_tmp}/${QT_LIB_NAME_tmp}.dll DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime)
        ENDIF ()
      ENDIF ()
    ENDFOREACH()
  ENDIF ()
ENDFUNCTION()
