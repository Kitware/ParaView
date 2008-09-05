# Exclude these
IF(PYTHON_EXECUTABLE AND VTK_HEADER_TESTING_PY)
  FOREACH(part Common Filters ServerManager)
    ADD_TEST(HeaderTesting-PVServer-${part} ${PYTHON_EXECUTABLE}
      ${VTK_HEADER_TESTING_PY}
      "${PVServer_SOURCE_DIR}/${part}" 
      vtkIceTConstants.h
      vtkMemberFunctionCommand.h
      vtkProcessModuleConnectionManagerInternals.h
      vtkPVServerOptionsInternals.h
      vtkPVTestUtities.h
      vtkSMPropertyInternals.h
      vtkSMPropertyInternals.h
      vtkSMProxyInternals.h
      vtkSMProxyManagerInternals.h
      vtkSMRepresentationStrategyVector.h
      vtkSpyPlotBlock.h
      vtkSpyPlotBlockIterator.h
      vtkSpyPlotIStream.h
      vtkSpyPlotReaderMap.h
      vtkSpyPlotUniReader.h
      vtkUndoStackInternal.h
      vtkSMPropertyHelper.h
      vtkInitializationHelper.h
      vtkPVTestUtilities.h
      vtkCTHFragmentCommBuffer.h
      vtkCTHFragmentIdList.h
      vtkCTHFragmentIdListItem.h
      vtkCTHFragmentPieceLoading.h
      vtkCTHFragmentPieceTransaction.h
      vtkCTHFragmentPieceTransactionMatrix.h
      vtkCTHFragmentProcessLoading.h
      vtkCTHFragmentProcessRing.h
      vtkCTHFragmentToProcMap.h)
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
