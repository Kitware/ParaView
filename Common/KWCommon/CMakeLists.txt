PROJECT(KWCommon)
INCLUDE_REGULAR_EXPRESSION("^(vtk|kw|icons).*$")

SET( KWCommon_SRCS
  vtkContainer.cxx
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

IF (NOT DO_NOT_BUILD_XML_RW)
  INCLUDE_DIRECTORIES(
    ${KWCommon_SOURCE_DIR}/XML
    )
  SET (KWCommon_SRCS ${KWCommon_SRCS}
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLActor2DReader.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLActor2DWriter.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLActorReader.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLActorWriter.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLCameraReader.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLCameraWriter.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLColorTransferFunctionReader.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLColorTransferFunctionWriter.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLIOBase.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLInteractorObserverReader.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLInteractorObserverWriter.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLLightReader.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLLightWriter.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLObjectReader.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLObjectWriter.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLPiecewiseFunctionReader.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLPiecewiseFunctionWriter.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLProp3DReader.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLProp3DWriter.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLPropReader.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLPropWriter.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLProperty2DReader.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLProperty2DWriter.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLPropertyReader.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLPropertyWriter.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLScalarBarActorReader.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLScalarBarActorWriter.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLScalarsToColorsReader.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLScalarsToColorsWriter.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLTextActorReader.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLTextActorWriter.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLTextPropertyReader.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLTextPropertyWriter.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLVolumePropertyReader.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLVolumePropertyWriter.cxx
    )
  IF (VTK_USE_HYBRID)
    SET (KWCommon_SRCS ${KWCommon_SRCS}
      ${KWCommon_SOURCE_DIR}/XML/vtkXML3DWidgetReader.cxx
      ${KWCommon_SOURCE_DIR}/XML/vtkXML3DWidgetWriter.cxx
      ${KWCommon_SOURCE_DIR}/XML/vtkXMLImagePlaneWidgetReader.cxx
      ${KWCommon_SOURCE_DIR}/XML/vtkXMLImagePlaneWidgetWriter.cxx
      ${KWCommon_SOURCE_DIR}/XML/vtkXMLPlaneWidgetReader.cxx
      ${KWCommon_SOURCE_DIR}/XML/vtkXMLPlaneWidgetWriter.cxx
      ${KWCommon_SOURCE_DIR}/XML/vtkXMLPolyDataSourceWidgetReader.cxx
      ${KWCommon_SOURCE_DIR}/XML/vtkXMLPolyDataSourceWidgetWriter.cxx
      ${KWCommon_SOURCE_DIR}/XML/vtkXMLScalarBarWidgetReader.cxx
      ${KWCommon_SOURCE_DIR}/XML/vtkXMLScalarBarWidgetWriter.cxx
      )
  ENDIF (VTK_USE_HYBRID)
  SET_SOURCE_FILES_PROPERTIES(
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLIOBase.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLObjectReader.cxx
    ${KWCommon_SOURCE_DIR}/XML/vtkXMLObjectWriter.cxx
    ABSTRACT
    )
ENDIF (NOT DO_NOT_BUILD_XML_RW)

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
    vtkRenderingTCL   
    vtkImaging
    vtkGraphics
    vtkIO
    vtkFiltering
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

  IF(BUILD_SHARED_LIBS)
    # Let parent project set KW_INSTALL_LIB_DIR to control library install
    # location.
    IF(NOT KW_INSTALL_LIB_DIR)
      SET(KW_INSTALL_LIB_DIR /lib)
    ENDIF(NOT KW_INSTALL_LIB_DIR)
    INSTALL_TARGETS(${KW_INSTALL_LIB_DIR} vtkKWCommonTCL)
  ENDIF(BUILD_SHARED_LIBS)

  IF(BUILD_TESTING)
    SUBDIRS(Testing)
  ENDIF(BUILD_TESTING)

  SUBDIRS(Utilities)
ENDIF(ParaView_SOURCE_DIR)

OPTION(USE_NOTEBOOK_ICONS "Display Notebook with icons." OFF)
MARK_AS_ADVANCED(USE_NOTEBOOK_ICONS)
