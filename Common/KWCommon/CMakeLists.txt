PROJECT(KWCommon)
INCLUDE_REGULAR_EXPRESSION("^(vtk|kw|icons).*$")

SET( KWCommon_SRCS
  vtkContainer.cxx
  vtkCornerAnnotation.cxx
  vtkKWArguments.cxx
  vtkKWDirectoryUtilities.cxx
  vtkKWEvent.cxx
  vtkKWIcon.cxx
  vtkKWMath.cxx
  vtkKWProcessStatistics.cxx
  vtkKWRegisteryUtilities.cxx
  vtkKWRemoteExecute.cxx
  vtkKWSerializer.cxx
  vtkQueue.cxx
  vtkString.cxx
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkContainer
  vtkKWEvent.cxx
  vtkQueue
  WRAP_EXCLUDE
  )

IF (WIN32)
  SET( KWCommon_SRCS ${KWCommon_SRCS}
    vtkKWWin32RegisteryUtilities.cxx
    )
  SET_SOURCE_FILES_PROPERTIES(
    vtkKWWin32RegisteryUtilities
    WRAP_EXCLUDE
    )
ELSE (WIN32)
  ADD_DEFINITIONS(-D_HPUX_SOURCE)
  SET( KWCommon_SRCS ${KWCommon_SRCS}
    vtkKWUNIXRegisteryUtilities.cxx
    )
  SET_SOURCE_FILES_PROPERTIES(
    vtkKWUNIXRegisteryUtilities
    WRAP_EXCLUDE
    )
ENDIF (WIN32)

IF(ParaView_SOURCE_DIR)
  IF(VTK_ZLIB_INCLUDE_DIRS)
    INCLUDE_DIRECTORIES(${VTK_ZLIB_INCLUDE_DIRS})
  ENDIF(VTK_ZLIB_INCLUDE_DIRS)

  ADD_LIBRARY(vtkKWCommon ${KWCommon_SRCS})
ELSE(ParaView_SOURCE_DIR)
  VTK_WRAP_TCL2(vtkKWCommonTCL KWCommonTCL_SRCS ${KWCommon_SRCS})
  ADD_LIBRARY(vtkKWCommonTCL ${KWCommonTCL_SRCS} ${KWCommon_SRCS})
ENDIF(ParaView_SOURCE_DIR)

IF(KWSYS_NAMESPACE)
  # Link to the kwsys library if the parent project configures it.
  IF(ParaView_SOURCE_DIR)
    TARGET_LINK_LIBRARIES (vtkKWCommon ${KWSYS_NAMESPACE})
  ELSE(ParaView_SOURCE_DIR)
    TARGET_LINK_LIBRARIES (vtkKWCommonTCL ${KWSYS_NAMESPACE})
  ENDIF(ParaView_SOURCE_DIR)
ENDIF(KWSYS_NAMESPACE)

IF(ParaView_SOURCE_DIR)
  TARGET_LINK_LIBRARIES (vtkKWCommon
    vtkFiltering
    vtkGenericFiltering
    vtkGraphics
    vtkIO
    vtkImaging
    vtkRendering
    )
ELSE(ParaView_SOURCE_DIR)
  TARGET_LINK_LIBRARIES (vtkKWCommonTCL
    vtkImagingTCL 
    vtkGraphicsTCL 
    vtkIOTCL
    vtkFilteringTCL
    vtkGenericFilteringTCL   
    vtkRenderingTCL   
    vtkImaging
    vtkGraphics
    vtkIO
    vtkFiltering
    vtkGenericFiltering
    vtkRendering
    )

  IF (NOT DO_NOT_BUILD_XML_RW)
    IF (VTK_USE_HYBRID)
      TARGET_LINK_LIBRARIES (vtkKWCommonTCL
        vtkHybridTCL 
        vtkHybrid
        )
    ENDIF (VTK_USE_HYBRID)
  ENDIF (NOT DO_NOT_BUILD_XML_RW)

ENDIF(ParaView_SOURCE_DIR)

SUBDIRS(Utilities)

INCLUDE_DIRECTORIES(${KWSYS_INCLUDE_DIR}
  ${KWCommon_BINARY_DIR}
  ${KWCommon_SOURCE_DIR}
  ${KWCOMMON_INCLUDE_DIR}
  )

IF(KWCOMMON_BUILD_PROCESSXML)
  ADD_EXECUTABLE(ProcessXML ProcessXML.cxx)
  TARGET_LINK_LIBRARIES(ProcessXML vtkCommon kwsys)
ENDIF(KWCOMMON_BUILD_PROCESSXML)

IF(BUILD_SHARED_LIBS)
  # Let parent project set KW_INSTALL_LIB_DIR to control library install
  # location.
  IF(NOT KW_INSTALL_LIB_DIR)
    SET(KW_INSTALL_LIB_DIR /lib)
  ENDIF(NOT KW_INSTALL_LIB_DIR)
  IF(ParaView_SOURCE_DIR)
    INSTALL_TARGETS(${KW_INSTALL_LIB_DIR} vtkKWCommon)
  ELSE(ParaView_SOURCE_DIR)
    INSTALL_TARGETS(${KW_INSTALL_LIB_DIR} vtkKWCommonTCL)
  ENDIF(ParaView_SOURCE_DIR)
ENDIF(BUILD_SHARED_LIBS)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)
