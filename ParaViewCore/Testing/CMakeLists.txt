# Exclude these
set (sub_dirs Common VTKExtensions ClientServerCore ServerImplementation ServerManager)
IF(PYTHON_EXECUTABLE AND VTK_HEADER_TESTING_PY)
  FOREACH(part ${sub_dirs})
    ADD_TEST(HeaderTesting-PVCore-${part} ${PYTHON_EXECUTABLE}
      ${VTK_HEADER_TESTING_PY}
      "${ParaViewCore_SOURCE_DIR}/${part}"
      vtkAMRDualContour.h
      vtkAMRDualGridHelper.h
      vtkIceTConstants.h
      vtkInitializationHelper.h
      vtkMaterialInterfaceCommBuffer.h
      vtkMaterialInterfaceIdList.h
      vtkMaterialInterfaceIdListItem.h
      vtkMaterialInterfacePieceLoading.h
      vtkMaterialInterfacePieceTransaction.h
      vtkMaterialInterfacePieceTransactionMatrix.h
      vtkMaterialInterfaceProcessLoading.h
      vtkMaterialInterfaceProcessRing.h
      vtkMaterialInterfaceToProcMap.h
      vtkMemberFunctionCommand.h
      vtkSIVectorPropertyTemplate.h
      vtkPriorityHelper.h
      vtkProcessModuleConnectionManagerInternals.h
      vtkProcessModuleInternals.h
      vtkPVDataDeliveryManager.h
      vtkPVPlugin.h
      vtkPVPluginTracker.h
      vtkPVPythonPluginInterface.h
      vtkPVServerManagerPluginInterface.h
      vtkPVServerOptionsInternals.h
      vtkPVTestUtities.h
      vtkSciVizStatisticsPrivate.h
      vtkSMCameraConfigurationFileInfo.h
      vtkSMMessage.h
      vtkSMMessageMinimal.h
      vtkSMPropertyHelper.h
      vtkSMPropertyInternals.h
      vtkSMPropertyInternals.h
      vtkSMProxyConfigurationFileInfo.h
      vtkSMProxyInternals.h
      vtkSMSessionProxyManagerInternals.h
      vtkSMRepresentationStrategyVector.h
      vtkSMUncheckedPropertyHelper.h
      vtkSMVectorPropertyTemplate.h
      vtkSpyPlotBlock.h
      vtkSpyPlotBlockIterator.h
      vtkSpyPlotHistoryReaderPrivate.h
      vtkSpyPlotIStream.h
      vtkSpyPlotReaderMap.h
      vtkSpyPlotUniReader.h
      vtkUndoStackInternal.h
      vtkReservedRemoteObjectIds.h
      vtkSMSessionProxyManager.h
      vtkPVSynchronizedRenderWindows.h
      )
  ENDFOREACH(part)
ENDIF(PYTHON_EXECUTABLE AND VTK_HEADER_TESTING_PY)

IF(TCL_TCLSH)
  IF(VTK_PRINT_SELF_CHECK_TCL)
    FOREACH(part ${sub_dirs})
      ADD_TEST(PrintSelf-PVCore-${part} ${TCL_TCLSH}
        ${VTK_PRINT_SELF_CHECK_TCL}
        ${ParaViewCore_SOURCE_DIR}/${part})
    ENDFOREACH(part)
  ENDIF(VTK_PRINT_SELF_CHECK_TCL)

  IF(VTK_FIND_STRING_TCL)
    FOREACH(part ${sub_dirs})
      ADD_TEST(TestSetObjectMacro-PVCore-${part} ${TCL_TCLSH}
        ${VTK_FIND_STRING_TCL}
        "${ParaViewCore_SOURCE_DIR}/${part}/vtk\\\\*.h"
        "vtkSetObjectMacro")
    ENDFOREACH(part)
  ENDIF(VTK_FIND_STRING_TCL)
ENDIF(TCL_TCLSH)
