SET( ParaView_SRCS
  vtkClientCompositeManager.cxx
  vtkCompleteArrays.cxx
  vtkCompositeProp.cxx
  vtkCTHData.cxx
  vtkCTHDataToPolyDataFilter.cxx
  vtkCTHExtractAMRPart.cxx
  vtkCTHFractal.cxx
  vtkCTHOutlineFilter.cxx
  vtkCTHSource.cxx
  vtkExtractCTHPart.cxx
  vtkKWBoundsDisplay.cxx
  vtkMultiDisplayManager.cxx
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
  vtkPVBatchCompositeManager.cxx
  vtkPVBoundsDisplay.cxx
  vtkPVBoxWidget.cxx
  vtkPVCalculatorWidget.cxx
  vtkPVCameraIcon.cxx
  vtkPVCameraManipulator.cxx
  vtkPVClassNameInformation.cxx
  vtkPVClientServerModule.cxx
  vtkPVColorMap.cxx
  vtkPVComponentSelection.cxx
  vtkPVCompositeBuffer.cxx
  vtkPVCompositePartDisplay.cxx
  vtkPVCompositeRenderModule.cxx
  vtkPVCompositeRenderModuleUI.cxx
  vtkPVCompositeUtilities.cxx
  vtkPVConnectDialog.cxx
  vtkPVContainerWidget.cxx
  vtkPVContourEntry.cxx 
  vtkPVContourWidgetProperty.cxx
  vtkPVCutEntry.cxx
  vtkPVDReaderModule.cxx
  vtkPVDWriter.cxx
  vtkPVData.cxx 
  vtkPVDataInformation.cxx
  vtkPVDataSetAttributesInformation.cxx
  vtkPVDataSetFileEntry.cxx
  vtkPVDataSetReaderModule.cxx
  ${PARAVIEW_SOURCE_DIR}/Filters/vtkPVDuplicatePolyData.cxx
  vtkPVDummyWidget.cxx
  vtkPVEnSightReaderModule.cxx
  vtkPVErrorLogDisplay.cxx
  vtkPVExtentEntry.cxx
  vtkPVExtentTranslator.cxx 
  vtkPVExtentWidgetProperty.cxx
  vtkPVExtractGeometryByScalar.cxx
  vtkPVExtractPartsWidget.cxx
  vtkPVFieldMenu.cxx
  vtkPVFileEntry.cxx
  vtkPVFileEntryProperty.cxx
  vtkPVGenericRenderWindowInteractor.cxx
  vtkPVGeometryFilter.cxx
  vtkPVGhostLevelDialog.cxx
  vtkPVGroupInputsWidget.cxx
  vtkPVImplicitPlaneWidget.cxx
  vtkPVIndexWidgetProperty.cxx
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
  vtkPVNumberOfOutputsInformation.cxx
  vtkPVObjectWidget.cxx
  vtkPVOrientScaleWidget.cxx
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
  vtkPVSaveBatchScriptDialog.cxx
  vtkPVScalarListWidgetProperty.cxx
  vtkPVScalarRangeLabel.cxx 
  vtkPVScale.cxx 
  vtkPVScaleFactorEntry.cxx
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
  vtkPVStringAndScalarListWidgetProperty.cxx
  vtkPVStringEntry.cxx
  vtkPVStringWidgetProperty.cxx
  vtkPVSummaryHelper.cxx
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
  vtkPVValueList.cxx
  vtkPVVectorEntry.cxx
  vtkPVWidget.cxx
  vtkPVWidgetCollection.cxx
  vtkPVWidgetProperty.cxx
  vtkPVWindow.cxx 
  vtkPVWindowResources.cxx 
  vtkPVWorldPointPicker.cxx 
  vtkPVWriter.cxx
  vtkPVXMLElement.cxx
  vtkPVXMLPackageParser.cxx
  vtkPVXMLParser.cxx
  vtkSelectInputs.cxx
  vtkStringList.cxx 
  vtkTiledDisplaySchedule.cxx
#  vtkWeightedRedistributePolyData.cxx
  vtkPVXDMFParameters.cxx
  vtkXDMFReaderModule.cxx
  vtkXMLCollectionReader.cxx
  vtkXMLPVAnimationWriter.cxx
  )

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

  SET(NOTFOUND 0)
  IF(NOT NETCDF_LIBRARY)
    SET(NOTFOUND 1)
    MESSAGE(SEND_ERROR "Cannot find NetCDF library")
  ENDIF(NOT NETCDF_LIBRARY)
  IF(NOT EXODUS_LIBRARY)
    SET(NOTFOUND 1)
    MESSAGE(SEND_ERROR "Cannot find Exodus library")
  ENDIF(NOT EXODUS_LIBRARY)
  IF(NOT EXODUS_SOURCE_DIR)
    SET(NOTFOUND 1)
    MESSAGE(SEND_ERROR "Cannot find Exodus source directory")
  ENDIF(NOT EXODUS_SOURCE_DIR)
  IF(NOT EXODUS_INCLUDE_DIR)
    SET(NOTFOUND 1)
    MESSAGE(SEND_ERROR "Cannot find Exodus include directory")
  ENDIF(NOT EXODUS_INCLUDE_DIR)

  IF(NOT NOTFOUND)
    INCLUDE(${EXODUS_SOURCE_DIR}/Files.cmake)
    INCLUDE_DIRECTORIES(${EXODUS_SOURCE_DIR})
    INCLUDE_DIRECTORIES(${EXODUS_INCLUDE_DIR})
    SET( ParaView_SRCS
      ${ParaView_SRCS}
      ${EXODUS_SRCS})
  ENDIF(NOT NOTFOUND)
ENDIF(PARAVIEW_USE_EXODUS)

OPTION(PARAVIEW_USE_SAF "Build using SAF library. Requires access to SAF libraries and HDF5 version 1.6 libraries" OFF)
MARK_AS_ADVANCED(PARAVIEW_USE_SAF)
IF(PARAVIEW_USE_SAF)                                                                                                                    
  IF(WIN32)
    FIND_FILE( SAF_LIBRARY safapi.lib )
    FIND_FILE( SAF_VBT_LIBRARY vbt.lib )
    FIND_FILE( SAF_DSL_LIBRARY dsl.lib )
  ELSE(WIN32)
    FIND_FILE( SAF_LIBRARY libsafapi.a )
    FIND_FILE( SAF_VBT_LIBRARY libvbt.a )
    FIND_FILE( SAF_DSL_LIBRARY libdsl.a )
  ENDIF(WIN32)

  FIND_PATH(SAF_INCLUDE_DIR saf.h)
  FIND_LIBRARY(SAF_READER_LIBRARY NAMES vtksnlSAFIO)
  FIND_PATH(VTKSNL_SOURCE_DIR vtkSnl.cmake )
  FIND_PATH(VTKSNL_BUILD_DIR vtksnlConfigure.h )

  SET(NOTFOUND 0)
  IF(NOT SAF_INCLUDE_DIR)
    SET(NOTFOUND 1)
    MESSAGE(SEND_ERROR "Cannot find SAF include directory")
  ENDIF(NOT SAF_INCLUDE_DIR)
  IF(NOT SAF_READER_LIBRARY)
    SET(NOTFOUND 1)
    MESSAGE(SEND_ERROR "Cannot find vtksnlSAFIO library")
  ENDIF(NOT SAF_READER_LIBRARY)
  IF(NOT VTKSNL_SOURCE_DIR)
    SET(NOTFOUND 1)
    MESSAGE(SEND_ERROR "Cannot find vtkSNL source directory")
  ENDIF(NOT VTKSNL_SOURCE_DIR)
  IF(NOT VTKSNL_BUILD_DIR)
    SET(NOTFOUND 1)
    MESSAGE(SEND_ERROR "Cannot find vtkSNL build directory")
  ENDIF(NOT VTKSNL_BUILD_DIR)
  IF(NOT SAF_LIBRARY)
    SET(NOTFOUND 1)
    MESSAGE(SEND_ERROR "Cannot find saf library")
  ENDIF(NOT SAF_LIBRARY)
  IF(NOT SAF_VBT_LIBRARY)
    SET(NOTFOUND 1)
    MESSAGE(SEND_ERROR "Cannot find saf vbt library")
  ENDIF(NOT SAF_VBT_LIBRARY)
  IF(NOT SAF_DSL_LIBRARY)
    SET(NOTFOUND 1)
    MESSAGE(SEND_ERROR "Cannot find saf dsl library")
  ENDIF(NOT SAF_DSL_LIBRARY)

  IF(NOT PARAVIEW_USE_SYSTEM_HDF5)
    SET(NOTFOUND 1)
    MESSAGE(SEND_ERROR "In order to build SAF, you must have hdf5 version 1.6, i.e. PARAVIEW_USE_SYSTEM_HDF5 must be true")
  ENDIF(NOT PARAVIEW_USE_SYSTEM_HDF5)

  IF(NOT NOTFOUND)
    INCLUDE_DIRECTORIES(${VTKSNL_SOURCE_DIR})
    INCLUDE_DIRECTORIES(${VTKSNL_SOURCE_DIR}/IO)
    INCLUDE_DIRECTORIES(${VTKSNL_SOURCE_DIR}/Graphics)
    INCLUDE_DIRECTORIES(${VTKSNL_SOURCE_DIR}/Common)
    INCLUDE_DIRECTORIES(${VTKSNL_SOURCE_DIR}/Utilities/Saf)
    INCLUDE_DIRECTORIES(${VTKSNL_BUILD_DIR})
    INCLUDE_DIRECTORIES(${SAF_INCLUDE_DIR})
    INCLUDE_DIRECTORIES(${SAF_INCLUDE_DIR}/private)
    SET( ParaView_SRCS
      ${ParaView_SRCS}
      ${VTKSNL_SOURCE_DIR}/IO/vtkSAFPVReader.cxx
      )
    SET_SOURCE_FILES_PROPERTIES(
      ${ParaView_SRCS}
      COMPILE_FLAGS -DBUNDLE_REGISTRY_INTO_EXECUTABLE
    )
  ENDIF(NOT NOTFOUND)
ENDIF(PARAVIEW_USE_SAF)

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
ENDIF(PARAVIEW_USE_EXODUS)

TARGET_LINK_LIBRARIES (vtkKWParaViewTCL
  vtkPVFiltersTCL
  vtkKWWidgetsTCL 
  vtkParallel)
IF(XDMF_FOUND)
  TARGET_LINK_LIBRARIES(vtkKWParaViewTCL vtkXdmfTCL)
ENDIF(XDMF_FOUND)
IF(PARAVIEW_USE_ICE_T)
  TARGET_LINK_LIBRARIES(vtkKWParaViewTCL icet icet_mpi icet_strategies)
  INCLUDE_DIRECTORIES(${ICE_T_INCLUDE_DIR})
ENDIF(PARAVIEW_USE_ICE_T)
IF(PARAVIEW_USE_SAF)
  TARGET_LINK_LIBRARIES( vtkKWParaViewTCL 
			 ${SAF_READER_LIBRARY} 
			 ${SAF_LIBRARY}
			 ${SAF_VBT_LIBRARY}
			 ${SAF_DSL_LIBRARY}
			 ${HDF5_LIBRARY} )
ENDIF(PARAVIEW_USE_SAF)

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

INCLUDE_DIRECTORIES(${PARAVIEW_BINARY_DIR}/ParaView
  ${PARAVIEW_SOURCE_DIR}/Filters)

SET(ParaViewBinary_SRCS
  ParaView.cxx)

IF(PARAVIEW_USE_SAF)
  SET(ParaViewBinary_SRCS ${ParaViewBinary_SRCS} 
    ${VTKSNL_SOURCE_DIR}/Graphics/vtkRectilinearGridToHexahedra.cxx
    ${VTKSNL_SOURCE_DIR}/Graphics/vtkStructuredGridToHexahedra.cxx
    ${VTKSNL_SOURCE_DIR}/IO/vtkSAFRectilinearReader.cxx
    ${VTKSNL_SOURCE_DIR}/IO/vtkSAFStructuredReader.cxx
    ${VTKSNL_SOURCE_DIR}/IO/vtkSAFUnstructuredReader.cxx
  )
ENDIF(PARAVIEW_USE_SAF)

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
