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

# Let parent project set KW_INSTALL_LIB_DIR to control library install
# location.
IF(NOT KW_INSTALL_LIB_DIR)
  SET(KW_INSTALL_LIB_DIR /lib)
ENDIF(NOT KW_INSTALL_LIB_DIR)
IF(NOT KW_INSTALL_BIN_DIR)
  SET(KW_INSTALL_BIN_DIR /bin)
ENDIF(NOT KW_INSTALL_BIN_DIR)

# Let parent project set KW_INSTALL_NO_DEVELOPMENT or KW_INSTALL_NO_RUNTIME
# to remove components from the installation.
SET(KW_INSTALL_NO_LIBRARIES)
IF(BUILD_SHARED_LIBS)
  IF(KW_INSTALL_NO_RUNTIME AND KW_INSTALL_NO_DEVELOPMENT)
    SET(KW_INSTALL_NO_LIBRARIES 1)
  ENDIF(KW_INSTALL_NO_RUNTIME AND KW_INSTALL_NO_DEVELOPMENT)
ELSE(BUILD_SHARED_LIBS)
  IF(KW_INSTALL_NO_DEVELOPMENT)
    SET(KW_INSTALL_NO_LIBRARIES 1)
  ENDIF(KW_INSTALL_NO_DEVELOPMENT)
ENDIF(BUILD_SHARED_LIBS)

INCLUDE_REGULAR_EXPRESSION("^(vtk|kw|icons).*$")

SET( KWCommon_SRCS
  vtkContainer.cxx
  vtkKWArguments.cxx
  vtkKWGenericMovieWriter.cxx
  vtkKWProcessStatistics.cxx
  vtkKWRemoteExecute.cxx
  vtkKWSerializer.cxx
  vtkQueue.cxx
  )

# include mpeg2 writer if the mpeg2 lib has been defined
IF (DEFINED vtkMPEG2Encode_INCLUDE_PATH)
  SET( KWCommon_SRCS ${KWCommon_SRCS} vtkMPEG2Writer.cxx)
  INCLUDE_DIRECTORIES(${vtkMPEG2Encode_INCLUDE_PATH})
  SET( KWCommon_LINK_LIBS ${KWCommon_LINK_LIBS} vtkMPEG2Encode)
ENDIF (DEFINED vtkMPEG2Encode_INCLUDE_PATH)

IF (WIN32)
  SET( KWCommon_SRCS ${KWCommon_SRCS} 
    vtkAVIWriter.cxx
    )
  SET( KWCommon_LINK_LIBS ${KWCommon_LINK_LIBS} vfw32)
ELSE (WIN32)
  ADD_DEFINITIONS(-D_HPUX_SOURCE)
ENDIF (WIN32)

IF(VTK_ZLIB_INCLUDE_DIRS)
  INCLUDE_DIRECTORIES(${VTK_ZLIB_INCLUDE_DIRS})
ENDIF(VTK_ZLIB_INCLUDE_DIRS)

ADD_LIBRARY(KWCommon ${KWCommon_SRCS})

IF(KWSYS_NAMESPACE)
  TARGET_LINK_LIBRARIES (KWCommon ${KWSYS_NAMESPACE})
ENDIF(KWSYS_NAMESPACE)

TARGET_LINK_LIBRARIES (KWCommon
  vtkFiltering
  vtkGenericFiltering
  vtkGraphics
  vtkIO
  vtkImaging
  vtkRendering
  ${KWCommon_LINK_LIBS}
  )

INCLUDE_DIRECTORIES(${KWSYS_INCLUDE_DIR}
  ${KWCommon_BINARY_DIR}
  ${KWCommon_SOURCE_DIR}
  )

SET(KW_EXE_SUFFIX)
SET(KW_EXE_INSTALL ${KW_INSTALL_BIN_DIR})
IF(KWCOMMON_BUILD_PROCESSXML)
  # For shared libraries on non-windows systems use the KWSys shared-forward
  # implementation to make sure shared libraries are found.
  IF(BUILD_SHARED_LIBS AND CMAKE_SKIP_RPATH)
    IF(NOT WIN32)
      SET(KW_FORWARD_DIR_BUILD ${EXECUTABLE_OUTPUT_PATH})
      SET(KW_FORWARD_BIN_TO_LIB "..${KW_INSTALL_LIB_DIR}")
      SET(KW_EXE_SUFFIX -real)
      SET(KW_FORWARD_EXE "kwProcessXML${KW_EXE_SUFFIX}")
      SET(KW_EXE_INSTALL ${KW_INSTALL_LIB_DIR})
      CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/kw-forward.c.in
                     ${CMAKE_CURRENT_BINARY_DIR}/ProcessXML-forward.c
                     @ONLY IMMEDIATE)
      ADD_EXECUTABLE(kwProcessXML
                     ${CMAKE_CURRENT_BINARY_DIR}/ProcessXML-forward.c)
      IF(NOT KW_INSTALL_NO_DEVELOPMENT)
        INSTALL_TARGETS(${KW_INSTALL_BIN_DIR} kwProcessXML)
      ENDIF(NOT KW_INSTALL_NO_DEVELOPMENT)
    ENDIF(NOT WIN32)
  ENDIF(BUILD_SHARED_LIBS AND CMAKE_SKIP_RPATH)

  ADD_EXECUTABLE(kwProcessXML${KW_EXE_SUFFIX} ProcessXML.cxx)
  TARGET_LINK_LIBRARIES(kwProcessXML${KW_EXE_SUFFIX} vtkCommon ${KWSYS_NAMESPACE})
  IF(NOT KW_INSTALL_NO_DEVELOPMENT)
    INSTALL_TARGETS(${KW_EXE_INSTALL} kwProcessXML${KW_EXE_SUFFIX})
  ENDIF(NOT KW_INSTALL_NO_DEVELOPMENT)
ENDIF(KWCOMMON_BUILD_PROCESSXML)

IF(NOT KW_INSTALL_NO_LIBRARIES)
  INSTALL_TARGETS(${KW_INSTALL_LIB_DIR} RUNTIME_DIRECTORY ${KW_INSTALL_BIN_DIR}
    KWCommon
    )
ENDIF(NOT KW_INSTALL_NO_LIBRARIES)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)
