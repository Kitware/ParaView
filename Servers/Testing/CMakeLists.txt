# Exclude these
IF(PYTHON_EXECUTABLE AND VTK_HEADER_TESTING_PY)
  FOREACH(part Common Filters ServerManager)
    ADD_TEST(HeaderTesting-PVServer-${part} ${PYTHON_EXECUTABLE}
      ${VTK_HEADER_TESTING_PY}
      "${PVServer_SOURCE_DIR}/${part}"
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
      vtkPriorityHelper.h
      vtkProcessModuleConnectionManagerInternals.h
      vtkPVPlugin.h
      vtkPVPythonPluginInterface.h
      vtkPVServerManagerPluginInterface.h
      vtkPVServerOptionsInternals.h
      vtkPVTestUtities.h
      vtkSciVizStatisticsPrivate.h
      vtkSMCameraConfigurationFileInfo.h
      vtkSMPropertyHelper.h
      vtkSMPropertyInternals.h
      vtkSMPropertyInternals.h
      vtkSMProxyConfigurationFileInfo.h
      vtkSMProxyInternals.h
      vtkSMProxyManagerInternals.h
      vtkSMRepresentationStrategyVector.h
      vtkSpyPlotBlock.h
      vtkSpyPlotBlockIterator.h
      vtkSpyPlotIStream.h
      vtkSpyPlotReaderMap.h
      vtkSpyPlotUniReader.h
      vtkUndoStackInternal.h
      )
  ENDFOREACH(part)
ENDIF(PYTHON_EXECUTABLE AND VTK_HEADER_TESTING_PY)

IF(TCL_TCLSH)
  IF(VTK_PRINT_SELF_CHECK_TCL)
    FOREACH(part Common Filters ServerManager)
      ADD_TEST(PrintSelf-PVServer-${part} ${TCL_TCLSH}
        ${VTK_PRINT_SELF_CHECK_TCL}
        ${PVServer_SOURCE_DIR}/${part})
    ENDFOREACH(part)
  ENDIF(VTK_PRINT_SELF_CHECK_TCL)

  IF(VTK_FIND_STRING_TCL)
    FOREACH(part Common Filters ServerManager)
      ADD_TEST(TestSetObjectMacro-PVServer-${part} ${TCL_TCLSH}
        ${VTK_FIND_STRING_TCL}
        "${PVServer_SOURCE_DIR}/${part}/vtk\\\\*.h"
        "vtkSetObjectMacro")
    ENDFOREACH(part)
  ENDIF(VTK_FIND_STRING_TCL)
ENDIF(TCL_TCLSH)
