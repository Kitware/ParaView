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

INCLUDE_REGULAR_EXPRESSION("^(vtk|kw|icons).*$")

SET( KWCommon_SRCS
  vtkContainer.cxx
  vtkCornerAnnotation.cxx
  vtkKWArguments.cxx
  vtkKWDirectoryUtilities.cxx
  vtkKWGenericMovieWriter.cxx
  vtkKWHistogram.cxx
  vtkKWHistogramSet.cxx
  vtkKWIcon.cxx
  vtkKWMath.cxx
  vtkKWProcessStatistics.cxx
  vtkKWRegisteryUtilities.cxx
  vtkKWRemoteExecute.cxx
  vtkKWSerializer.cxx
  vtkQueue.cxx
  vtkString.cxx
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
    vtkKWWin32RegisteryUtilities.cxx
    )
  SET( KWCommon_LINK_LIBS ${KWCommon_LINK_LIBS} vfw32)
ELSE (WIN32)
  ADD_DEFINITIONS(-D_HPUX_SOURCE)
  SET( KWCommon_SRCS ${KWCommon_SRCS}
    vtkKWUNIXRegisteryUtilities.cxx
    )
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

SUBDIRS(Utilities)

INCLUDE_DIRECTORIES(${KWSYS_INCLUDE_DIR}
  ${KWCommon_BINARY_DIR}
  ${KWCommon_SOURCE_DIR}
  ${KWCOMMON_INCLUDE_DIR}
  )

IF(KWCOMMON_BUILD_PROCESSXML)
  ADD_EXECUTABLE(kwProcessXML ProcessXML.cxx)
  TARGET_LINK_LIBRARIES(kwProcessXML vtkCommon kwsys)
  INSTALL_TARGETS(/bin kwProcessXML)
ENDIF(KWCOMMON_BUILD_PROCESSXML)

IF(BUILD_SHARED_LIBS)
  # Let parent project set KW_INSTALL_LIB_DIR to control library install
  # location.
  IF(NOT KW_INSTALL_LIB_DIR)
    SET(KW_INSTALL_LIB_DIR /lib)
  ENDIF(NOT KW_INSTALL_LIB_DIR)
  INSTALL_TARGETS(${KW_INSTALL_LIB_DIR} KWCommon)
ENDIF(BUILD_SHARED_LIBS)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)
