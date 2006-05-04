PROJECT(KWCommon)

#############################################################################
# to use this library in your code you will need to SUBDIR into this
# directory so that it gets built and then you will use the following variables
# in your CMakeLists files to get the proper include paths and libraries
SET (KWCommon_INCLUDE_PATH 
  "${CMAKE_CURRENT_SOURCE_DIR}"
  CACHE INTERNAL "include paths for KWCommon"
  )

SET (KWCommon_LIBRARIES KWCommon CACHE INTERNAL "libraries for KWCommon")
#############################################################################
# Find an external package VTK if we are not being built as a part of a larger
# project that includes VTK as a subdir.
MACRO( INCLUDE_PACKAGE PACKAGE_NAME )
  IF( NOT ${PACKAGE_NAME}_SOURCE_DIR )
    FIND_PACKAGE( ${PACKAGE_NAME} REQUIRED )
    IF( ${PACKAGE_NAME}_FOUND )
      INCLUDE( ${${PACKAGE_NAME}_USE_FILE} )
    ENDIF( ${PACKAGE_NAME}_FOUND )
  ENDIF( NOT ${PACKAGE_NAME}_SOURCE_DIR )
ENDMACRO( INCLUDE_PACKAGE )

INCLUDE_PACKAGE( VTK )
########################################################################

# Let parent project set KWCommon_INSTALL_LIB_DIR to control library install
# location.
IF(NOT KWCommon_INSTALL_LIB_DIR)
  SET(KWCommon_INSTALL_LIB_DIR "/lib/${PROJECT_NAME}")
ENDIF(NOT KWCommon_INSTALL_LIB_DIR)
IF(NOT KWCommon_INSTALL_BIN_DIR)
  SET(KWCommon_INSTALL_BIN_DIR /bin)
ENDIF(NOT KWCommon_INSTALL_BIN_DIR)

# Let parent project set KWCommon_INSTALL_NO_DEVELOPMENT or 
# KWCommon_INSTALL_NO_RUNTIME to remove components from the installation.
SET(KWCommon_INSTALL_NO_LIBRARIES)
IF(BUILD_SHARED_LIBS)
  IF(KWCommon_INSTALL_NO_RUNTIME AND KWCommon_INSTALL_NO_DEVELOPMENT)
    SET(KWCommon_INSTALL_NO_LIBRARIES 1)
  ENDIF(KWCommon_INSTALL_NO_RUNTIME AND KWCommon_INSTALL_NO_DEVELOPMENT)
ELSE(BUILD_SHARED_LIBS)
  IF(KWCommon_INSTALL_NO_DEVELOPMENT)
    SET(KWCommon_INSTALL_NO_LIBRARIES 1)
  ENDIF(KWCommon_INSTALL_NO_DEVELOPMENT)
ENDIF(BUILD_SHARED_LIBS)

INCLUDE_REGULAR_EXPRESSION("^(vtk|kw|icons).*$")

SET( KWCommon_SRCS
  vtkContainer.cxx
  vtkKWProcessStatistics.cxx
  vtkKWRemoteExecute.cxx
  vtkQueue.cxx
  )

IF(VTK_ZLIB_INCLUDE_DIRS)
  INCLUDE_DIRECTORIES(${VTK_ZLIB_INCLUDE_DIRS})
ENDIF(VTK_ZLIB_INCLUDE_DIRS)

ADD_LIBRARY(KWCommon ${KWCommon_SRCS})
TARGET_LINK_LIBRARIES (KWCommon
  vtkFiltering
  vtkGenericFiltering
  vtkGraphics
  vtkIO
  vtkImaging
  vtkRendering
  vtksys
  ${KWCommon_LINK_LIBS}
  )

INCLUDE_DIRECTORIES(
  ${KWCommon_BINARY_DIR}
  ${KWCommon_SOURCE_DIR}
  )

SET(KWCommon_EXE_SUFFIX)
SET(KWCommon_EXE_INSTALL ${KWCommon_INSTALL_BIN_DIR})
IF(KWCOMMON_BUILD_PROCESSXML)
  # For shared libraries on non-windows systems use the KWSys shared-forward
  # implementation to make sure shared libraries are found.
  IF(BUILD_SHARED_LIBS AND CMAKE_SKIP_RPATH)
    IF(NOT WIN32)
      SET(KWCommon_FORWARD_DIR_BUILD ${EXECUTABLE_OUTPUT_PATH})
      SET(KWCommon_FORWARD_BIN_TO_LIB "..${KWCommon_INSTALL_LIB_DIR}")
      SET(KWCommon_EXE_SUFFIX -real)
      SET(KWCommon_FORWARD_EXE "kwProcessXML${KWCommon_EXE_SUFFIX}")
      SET(KWCommon_EXE_INSTALL ${KWCommon_INSTALL_LIB_DIR})
      CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/kw-forward.c.in
                     ${CMAKE_CURRENT_BINARY_DIR}/ProcessXML-forward.c
                     @ONLY IMMEDIATE)
      ADD_EXECUTABLE(kwProcessXML
                     ${CMAKE_CURRENT_BINARY_DIR}/ProcessXML-forward.c)
      IF(NOT KWCommon_INSTALL_NO_DEVELOPMENT)
        INSTALL_TARGETS(${KWCommon_INSTALL_BIN_DIR} kwProcessXML)
      ENDIF(NOT KWCommon_INSTALL_NO_DEVELOPMENT)
    ENDIF(NOT WIN32)
  ENDIF(BUILD_SHARED_LIBS AND CMAKE_SKIP_RPATH)

  ADD_EXECUTABLE(kwProcessXML${KWCommon_EXE_SUFFIX} ProcessXML.cxx)
  TARGET_LINK_LIBRARIES(kwProcessXML${KWCommon_EXE_SUFFIX} vtkCommon vtksys)
  IF(NOT KWCommon_INSTALL_NO_DEVELOPMENT)
    INSTALL_TARGETS(${KWCommon_EXE_INSTALL} kwProcessXML${KWCommon_EXE_SUFFIX})
  ENDIF(NOT KWCommon_INSTALL_NO_DEVELOPMENT)
  IF(KWCommon_EXE_SUFFIX)
    ADD_DEPENDENCIES(kwProcessXML kwProcessXML${KWCommon_EXE_SUFFIX})
  ENDIF(KWCommon_EXE_SUFFIX)
ENDIF(KWCOMMON_BUILD_PROCESSXML)

IF(NOT KWCommon_INSTALL_NO_LIBRARIES)
  INSTALL_TARGETS(${KWCommon_INSTALL_LIB_DIR} 
    RUNTIME_DIRECTORY ${KWCommon_INSTALL_BIN_DIR}
    ${KWCommon_LIBRARIES})
ENDIF(NOT KWCommon_INSTALL_NO_LIBRARIES)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)

# Install include tree
# IF(NOT KWCommon_INSTALL_NO_DEVELOPMENT)
#  INSTALL_FILES(/include/KWCommon "(\\.h|\\.txx)$")
# ENDIF(NOT KWCommon_INSTALL_NO_DEVELOPMENT)

SUBDIRS( CMake )

