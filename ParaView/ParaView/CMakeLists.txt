SET( ParaView_SRCS
  vtkClientCompositeManager.cxx
  vtkColorByPart.cxx
  vtkCompleteArrays.cxx
  vtkCompositeProp.cxx
  vtkCTHData.cxx
  vtkCTHDataToPolyDataFilter.cxx
  vtkCTHExtractAMRPart.cxx
  vtkCTHFractal.cxx
  vtkCTHOutlineFilter.cxx
  vtkCTHSource.cxx
  vtkExtractCTHPart.cxx
  vtkGroup.cxx
  vtkKWBoundsDisplay.cxx
  vtkKWExtractGeometryByScalar.cxx
  vtkMergeArrays.cxx
  vtkMultiDisplayManager.cxx
  vtkMultiOut.cxx
  vtkMultiOut2.cxx
  vtkMultiOut3.cxx
  vtkPV3DWidget.cxx
  vtkPVAdvancedReaderModule.cxx
  vtkPVAnimationInterface.cxx
  vtkPVAnimationInterfaceEntry.cxx
  vtkPVApplication.cxx 
  vtkPVApplicationResources.cxx 
  vtkPVApplicationSettingsInterface.cxx
  vtkPVArrayInformation.cxx
  vtkPVArrayMenu.cxx 
  vtkPVArraySelection.cxx
  vtkPVArrowSource.cxx
  vtkPVBatchCompositeManager.cxx
  vtkPVBoundsDisplay.cxx
  vtkPVBoxWidget.cxx
  vtkPVCalculatorWidget.cxx
  vtkPVCameraIcon.cxx
  vtkPVCameraManipulator.cxx
  vtkPVClientServerModule.cxx
  vtkPVClipDataSet.cxx
  vtkPVColorMap.cxx
  vtkPVComponentSelection.cxx
  vtkPVCompositeBuffer.cxx
  vtkPVCompositePartDisplay.cxx
  vtkPVCompositeRenderModule.cxx
  vtkPVCompositeRenderModuleUI.cxx
  vtkPVCompositeUtilities.cxx
  vtkPVConnectDialog.cxx
  vtkPVConnectivityFilter.cxx
  vtkPVContainerWidget.cxx
  vtkPVContourEntry.cxx 
  vtkPVContourFilter.cxx
  vtkPVDReaderModule.cxx
  vtkPVDWriter.cxx
  vtkPVData.cxx 
  vtkPVDataInformation.cxx
  vtkPVDataSetAttributesInformation.cxx
  vtkPVDataSetFileEntry.cxx
  vtkPVDataSetReaderModule.cxx
  vtkPVDummyWidget.cxx
  vtkPVDuplicatePolyData.cxx
  vtkPVEnSightMasterServerReader.cxx
  vtkPVEnSightMasterServerTranslator.cxx
  vtkPVEnSightReaderModule.cxx
  vtkPVErrorLogDisplay.cxx
  vtkPVExtentEntry.cxx
  vtkPVExtentTranslator.cxx 
  vtkPVExtractGeometryByScalar.cxx
  vtkPVExtractPartsWidget.cxx
  vtkPVFieldMenu.cxx
  vtkPVFileEntry.cxx
  vtkPVGenericRenderWindowInteractor.cxx
  vtkPVGeometryFilter.cxx
  vtkPVGhostLevelDialog.cxx
  vtkPVGlyphFilter.cxx
  vtkPVGroupInputsWidget.cxx
  vtkPVImageContinuousDilate3D.cxx
  vtkPVImageContinuousErode3D.cxx
  vtkPVImageGradient.cxx
  vtkPVImageGradientMagnitude.cxx
  vtkPVImageMedian3D.cxx
  vtkPVImplicitPlaneWidget.cxx
  vtkPVInformation.cxx
  vtkPVInitialize.cxx
  vtkPVInputArrayRequirement.cxx
  vtkPVInputFixedTypeRequirement.cxx
  vtkPVInputGroupRequirement.cxx
  vtkPVInputMenu.cxx 
  vtkPVInputProperty.cxx
  vtkPVInputRequirement.cxx
  vtkPVInteractorStyle.cxx
  vtkPVInteractorStyleCenterOfRotation.cxx
  vtkPVInteractorStyleControl.cxx
  vtkPVInteractorStyleFly.cxx
  vtkPVJoystickFly.cxx
  vtkPVJoystickFlyIn.cxx
  vtkPVJoystickFlyOut.cxx
  vtkPVLODActor.cxx
  vtkPVLODPartDisplay.cxx
  vtkPVLODPartDisplayInformation.cxx
  vtkPVLODRenderModule.cxx
  vtkPVLODRenderModuleUI.cxx
  vtkPVLabel.cxx
  vtkPVLabeledToggle.cxx
  vtkPVLineSourceWidget.cxx
  vtkPVLineWidget.cxx
  vtkPVMPIProcessModule.cxx
  vtkPVMPIRenderModule.cxx
  vtkPVMPIRenderModuleUI.cxx
  vtkPVMinMax.cxx
  vtkPVMultiDisplayPartDisplay.cxx
  vtkPVMultiDisplayRenderModule.cxx
  vtkPVMultiDisplayRenderModuleUI.cxx
  vtkPVNavigationWindow.cxx
  vtkPVObjectWidget.cxx
  vtkPVPart.cxx
  vtkPVPartDisplay.cxx
  vtkPVPLOT3DReaderModule.cxx
  vtkPVPivotManipulator.cxx
  vtkPVPlaneWidget.cxx
  vtkPVPointSourceWidget.cxx
  vtkPVPointWidget.cxx
  vtkPVProbe.cxx
  vtkPVProcessModule.cxx
  vtkPVPushButton.cxx
  vtkPVRawReaderModule.cxx
  vtkPVReaderModule.cxx
#  vtkPVRenderGroupDialog.cxx
  vtkPVRenderModule.cxx
  vtkPVRenderModuleUI.cxx
  vtkPVRenderView.cxx 
  vtkPVRibbonFilter.cxx
  vtkPVSaveBatchScriptDialog.cxx
  vtkPVScalarRangeLabel.cxx 
  vtkPVScale.cxx 
  vtkPVSelectCTHArrays.cxx
  vtkPVSelectCustomReader.cxx
  vtkPVSelectTimeSet.cxx
  vtkPVSelectWidget.cxx 
  vtkPVSelectionList.cxx
  vtkPVServerFileDialog.cxx
  vtkPVSource.cxx 
  vtkPVSourceCollection.cxx
  vtkPVSourceList.cxx
  vtkPVSourceWidget.cxx
  vtkPVSourcesNavigationWindow.cxx
  vtkPVSphereWidget.cxx
  vtkPVStringEntry.cxx
  vtkPVSummaryHelper.cxx
  vtkPVThresholdFilter.cxx
  vtkPVTiledDisplayManager.cxx
  vtkPVTimerInformation.cxx
  vtkPVTimerLogDisplay.cxx
  vtkPVTraceFileDialog.cxx
  vtkPVTrackballMoveActor.cxx
  vtkPVTrackballPan.cxx
  vtkPVTrackballRoll.cxx
  vtkPVTrackballRotate.cxx
  vtkPVTrackballZoom.cxx
  vtkPVTreeComposite.cxx 
  vtkPVUpdateSuppressor.cxx
  vtkPVVectorEntry.cxx
  vtkPVWarpScalar.cxx
  vtkPVWarpVector.cxx
  vtkPVWidget.cxx
  vtkPVWidgetCollection.cxx
  vtkPVWindow.cxx 
  vtkPVWindowResources.cxx 
  vtkPVWorldPointPicker.cxx 
  vtkPVWriter.cxx
  vtkPVXMLElement.cxx
  vtkPVXMLPackageParser.cxx
  vtkPVXMLParser.cxx
#  vtkRedistributePolyData.cxx
  vtkSelectInputs.cxx
  vtkStringList.cxx 
  vtkStructuredCacheFilter.cxx
  vtkTiledDisplaySchedule.cxx
  vtkVRMLSource.cxx
#  vtkWeightedRedistributePolyData.cxx
  vtkPVXDMFParameters.cxx
  vtkXDMFReaderModule.cxx
  vtkXMLCollectionReader.cxx
  vtkXMLPVAnimationWriter.cxx
  vtkXMLPVDWriter.cxx
  )

IF(VTK_USE_MPI)
  SET(ParaView_SRCS
    ${ParaView_SRCS}
    vtkDistributedDataFilter.cxx
    vtkPKdTree.cxx
    vtkExtractCells.cxx
    vtkKdTree.cxx
    vtkMergeCells.cxx
    vtkPlanesIntersection.cxx
    vtkPointsProjectedHull.cxx
#  vtkAllToNRedistributePolyData.cxx
#  vtkBalancedRedistributePolyData.cxx
    )
ENDIF(VTK_USE_MPI)


IF (PARAVIEW_USE_ICE_T)
  SET( ParaView_SRCS
    ${ParaView_SRCS}
    vtkPVDeskTopRenderModule.cxx
    vtkPVDeskTopRenderModuleUI.cxx
    vtkDesktopDeliveryClient.cxx
    vtkDesktopDeliveryServer.cxx
    #vtkIceTComposite.cxx
    vtkIceTRenderer.cxx
    vtkIceTRenderManager.cxx
    vtkParallelRenderManager.cxx
    vtkIceTFactory.cxx
    vtkCompositeRenderManager.cxx
    vtkPVIceTDisplayRenderModuleUI.cxx
    vtkPVIceTDisplayRenderModule.cxx
    vtkPVIceTRenderModuleUI.cxx
    vtkPVIceTRenderModule.cxx
    vtkIceTClientCompositeManager.cxx
    )
  SET_SOURCE_FILES_PROPERTIES(
    vtkParallelRenderManager.cxx
    vtkIceTFactory.cxx
    ABSTRACT
    )
ENDIF (PARAVIEW_USE_ICE_T)

IF (VTK_USE_PATENTED)
  SET( ParaView_SRCS
    ${ParaView_SRCS}
    vtkPVKitwareContourFilter.cxx)
ENDIF (VTK_USE_PATENTED)

IF(NOT PARAVIEW_NO_HDF5)
  SET(ParaView_SRCS ${ParaView_SRCS} vtkHDF5RawImageReader.cxx)
ENDIF(NOT PARAVIEW_NO_HDF5)

SET_SOURCE_FILES_PROPERTIES(
  vtkPVApplicationResources
  vtkPVWindowResources
  WRAP_EXCLUDE
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkPVObjectWidget
  vtkPVWidget
  vtkPV3DWidget
  vtkPVJoystickFly
  vtkPVSourceWidget
  ABSTRACT
  )

OPTION(PARAVIEW_USE_EXODUS "Build using Exodus library. Requires access to Exodus libraries" OFF)
MARK_AS_ADVANCED(PARAVIEW_USE_EXODUS)
IF(PARAVIEW_USE_EXODUS)
  FIND_PATH(EXODUS_SOURCE_DIR 
    vtkExodusReader.h
    ../Trilab/ExodusReader
    ../../Trilab/ExodusReader
    ../../../Trilab/ExodusReader)
  FIND_PATH(EXODUS_INCLUDE_DIR
    exodusII.h
    ../Exodus/inc
    ../../Exodus/inc
    ../../../Exodus/inc)
  FIND_LIBRARY(EXODUS_LIBRARY
    NAMES exoIIv2c exodusII
    PATHS
    ../Exodus/lib
    ../../Exodus/lib
    ../../../Exodus/lib)
  FIND_LIBRARY(NETCDF_LIBRARY
    NAMES netcdf
    PATHS 
    ../Exodus/lib
    ../../Exodus/lib
    ../../../Exodus/lib)

  SET(NOTFOUD 0)
  IF(NOT NETCDF_LIBRARY)
    SET(NOTFOUD 1)
    MESSAGE(SEND_ERROR "Cannot find NetCDF library")
  ENDIF(NOT NETCDF_LIBRARY)
  IF(NOT EXODUS_LIBRARY)
    SET(NOTFOUD 1)
    MESSAGE(SEND_ERROR "Cannot find Exodus library")
  ENDIF(NOT EXODUS_LIBRARY)
  IF(NOT EXODUS_SOURCE_DIR)
    SET(NOTFOUD 1)
    MESSAGE(SEND_ERROR "Cannot find Exodus source directory")
  ENDIF(NOT EXODUS_SOURCE_DIR)
  IF(NOT EXODUS_INCLUDE_DIR)
    SET(NOTFOUD 1)
    MESSAGE(SEND_ERROR "Cannot find Exodus include directory")
  ENDIF(NOT EXODUS_INCLUDE_DIR)

  IF(NOT NOTFOUD)
    INCLUDE(${EXODUS_SOURCE_DIR}/Files.cmake)
    INCLUDE_DIRECTORIES(${EXODUS_SOURCE_DIR})
    INCLUDE_DIRECTORIES(${EXODUS_INCLUDE_DIR})
    SET( ParaView_SRCS
      ${ParaView_SRCS}
      ${EXODUS_SRCS})
  ENDIF(NOT NOTFOUD)
ENDIF(PARAVIEW_USE_EXODUS)


# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR2(vtkParaViewInstantiator PVInstantiator_SRCS
  ${ParaView_SRCS}
  EXPORT_MACRO VTK_EXPORT
  HEADER_LOCATION ${PARAVIEW_BINARY_DIR}/ParaView)

VTK_WRAP_TCL2(vtkKWParaViewTCL ParaViewTCL_SRCS ${PVInstantiator_SRCS} 
  ${ParaView_SRCS})
ADD_LIBRARY(vtkKWParaViewTCL ${ParaViewTCL_SRCS} ${PVInstantiator_SRCS} 
  ${ParaView_SRCS})
IF(PARAVIEW_USE_EXODUS)
  TARGET_LINK_LIBRARIES(vtkKWParaViewTCL ${EXODUS_LIBRARY} ${NETCDF_LIBRARY})
  IF(WIN32)
    TARGET_LINK_LIBRARIES(vtkKWParaViewTCL -NODEFAULTLIB:libcmt -NODEFAULTLIB:libc -NODEFAULTLIB:msvcrtd msvcrtd.lib libcd.lib libcmtd.lib)
  ENDIF(WIN32)
ENDIF(PARAVIEW_USE_EXODUS)

TARGET_LINK_LIBRARIES (vtkKWParaViewTCL
  vtkKWWidgetsTCL 
  vtkParallel)
IF(XDMF_FOUND)
  TARGET_LINK_LIBRARIES(vtkKWParaViewTCL vtkXdmfTCL)
ENDIF(XDMF_FOUND)
IF(PARAVIEW_USE_ICE_T)
  TARGET_LINK_LIBRARIES(vtkKWParaViewTCL icet icet_mpi icet_strategies)
  INCLUDE_DIRECTORIES(${ICE_T_INCLUDE_DIR})
ENDIF(PARAVIEW_USE_ICE_T)


CONFIGURE_FILE(
  ${PARAVIEW_SOURCE_DIR}/ParaView/vtkPVSourceInterfaceDirectories.h.in
  ${PARAVIEW_BINARY_DIR}/ParaView/vtkPVSourceInterfaceDirectories.h
  ESCAPE_QUOTES)

CONFIGURE_FILE(${PARAVIEW_SOURCE_DIR}/ParaView/vtkPVDemoPaths.h.in
  ${PARAVIEW_BINARY_DIR}/ParaView/vtkPVDemoPaths.h
  ESCAPE_QUOTES)

CONFIGURE_FILE(${PARAVIEW_SOURCE_DIR}/ParaView/vtkPVHelpPaths.h.in
  ${PARAVIEW_BINARY_DIR}/ParaView/vtkPVHelpPaths.h
  ESCAPE_QUOTES)

CONFIGURE_FILE(${PARAVIEW_SOURCE_DIR}/ParaView/vtkPVConfig.h.in
  ${PARAVIEW_BINARY_DIR}/ParaView/vtkPVConfig.h
  ESCAPE_QUOTES)

INCLUDE_DIRECTORIES(${PARAVIEW_BINARY_DIR}/ParaView)

SET(ParaViewBinary_SRCS
  ParaView.cxx)

IF(WIN32)
  SET(ParaViewBinary_SRCS ${ParaViewBinary_SRCS} ParaView.rc)
ENDIF(WIN32)

ADD_EXECUTABLE(paraview WIN32 ${ParaViewBinary_SRCS}) 
IF(VTK_USE_CARBON)
  FIND_PROGRAM(VTK_APPLE_RESOURCE Rez /Developer/Tools)
  IF(VTK_APPLE_RESOURCE)
    ADD_CUSTOM_COMMAND(
      SOURCE paraview
      COMMAND ${VTK_APPLE_RESOURCE}
      ARGS Carbon.r -o ${EXECUTABLE_OUTPUT_PATH}/paraview
      TARGET paraview
      )
  ENDIF(VTK_APPLE_RESOURCE)
ENDIF(VTK_USE_CARBON)

TARGET_LINK_LIBRARIES (paraview
  vtkKWParaViewTCL)

IF(UNIX)
  IF(NOT APPLE)
    ADD_EXECUTABLE(ParaView OldParaView.cxx)
  ENDIF(NOT APPLE)
ENDIF(UNIX)


IF(NOT PARAVIEW_NO_HDF5)
  TARGET_LINK_LIBRARIES(vtkKWParaViewTCL ${PARAVIEW_HDF5_LIBRARIES})
ENDIF(NOT PARAVIEW_NO_HDF5)

ADD_EXECUTABLE(ProcessXML ProcessXML.cxx)
TARGET_LINK_LIBRARIES(ProcessXML vtkCommon)

SET(EXE_EXT "")
IF(WIN32)
  SET(EXE_EXT ".exe")
ENDIF(WIN32)

SET(MAKE_SYSTEM)
IF(CMAKE_BUILD_TOOL MATCHES "make")
  SET(MAKE_SYSTEM 1)
ENDIF(CMAKE_BUILD_TOOL MATCHES "make")

SET(CFG_INIT "/${CMAKE_CFG_INTDIR}")
IF(MAKE_SYSTEM OR UNIX)
  SET(CFG_INIT "")
ENDIF(MAKE_SYSTEM OR UNIX)
SET(CMD ${EXECUTABLE_OUTPUT_PATH}${CFG_INIT}/ProcessXML${EXE_EXT})

SET(cmver "${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")
IF("${cmver}" GREATER 1.6)
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vtkPVGeneratedModules.h
    DEPENDS 
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Filters.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Manipulators.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Sources.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Readers.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Writers.xml
    ProcessXML
    COMMAND ${CMD}
    ARGS ${CMAKE_CURRENT_BINARY_DIR}/vtkPVGeneratedModules.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources
    Filters Manipulators Readers Sources Writers)
ELSE("${cmver}" GREATER 1.6)
  ADD_CUSTOM_COMMAND(
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/ProcessXML.cxx
    TARGET vtkKWParaViewTCL
    DEPENDS 
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Filters.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Manipulators.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Sources.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Readers.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Writers.xml
    ${CMD}
    OUTPUTS ${CMAKE_CURRENT_BINARY_DIR}/vtkPVGeneratedModules.h
    COMMAND ${CMD}
    ARGS ${CMAKE_CURRENT_BINARY_DIR}/vtkPVGeneratedModules.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources
    Filters Manipulators Readers Sources Writers)
  ADD_DEPENDENCIES(vtkKWParaViewTCL ProcessXML)
ENDIF("${cmver}" GREATER 1.6)

SET_SOURCE_FILES_PROPERTIES(vtkPVInitialize.cxx PROPERTIES OBJECT_DEPENDS 
  ${CMAKE_CURRENT_BINARY_DIR}/vtkPVGeneratedModules.h)

INCLUDE (${PARAVIEW_BINARY_DIR}/ParaView/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${PARAVIEW_SOURCE_DIR}/ParaView/LocalUserOptions.cmake OPTIONAL)

IF(BUILD_SHARED_LIBS)
  INSTALL_TARGETS(${KW_INSTALL_LIB_DIR} vtkKWParaViewTCL)
ENDIF(BUILD_SHARED_LIBS)

INSTALL_TARGETS(/bin paraview)

#OUTPUT_REQUIRED_FILES(${PARAVIEW_SOURCE_DIR}/ParaView/ParaView.cxx 
#                      ${PARAVIEW_BINARY_DIR}/ReqList.txt)
