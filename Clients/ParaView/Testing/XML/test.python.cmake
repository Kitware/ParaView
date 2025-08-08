# Tests that require python

list(APPEND TESTS_WITHOUT_BASELINES
  AllPropertiesSaveStatePython.xml
  FullNotation.xml
  IconBrowser.xml
  MacroEditor.xml
  MultipleNumberOfComponents.xml
  ProgrammableSourcePythonEditorLink.xml
  PythonDefaultLoadState.xml
  PythonDefaultSaveState.xml
  PythonEditorTab.xml
  PythonResetSessionMacro.xml
  SpreadSheetNullArrayName.xml # needs programmable filter
  # TestPopOutWidget.xml
  TestPythonConsole.xml
  TraceIntegrateVariables.xml
  )

# This test rely on copy being Ctrl-C
if(NOT APPLE)
  list(APPEND TESTS_WITHOUT_BASELINES
    TooltipCopy.xml
    TestHTGHoverOnCell.xml
    )
endif()

# VTTKJS exporter requires the Web module
if (PARAVIEW_ENABLE_WEB)
  list (APPEND TESTS_WITHOUT_BASELINES
    ExportToVTKJS.xml
    ExportToVTKJSWithArraySelection.xml
  )
  set (ExportToVTKJS_FORCE_SERIAL TRUE) # since this uses popup-menu
endif ()

list(APPEND TESTS_WITH_BASELINES
  AutoSaveState.xml
  ColorByComponentNames.xml # needs programmable filter
  LiveProgrammableSource.xml
  LinkRenderViews.xml
  LinkViews.xml
  PythonShellRunScript.xml
  PythonEditorRun.xml
  SaveLoadStatePython.xml
  SaveLoadStateSelectionPython.xml
  TraceExodus.xml
  TraceExportAndSaveData.xml
  TraceSaveGeometry.xml
  # TraceStatisticsFilter.xml (see paraview/paraview#20661. Also was disabled in CS/CRS mode)
  TraceSupplementalProxiesFully.xml
  TraceTimeControls.xml
  TraceWithoutRenderingComponents.xml
  )
# Surface selection unstable on CRS mode
set(SaveLoadStateSelectionPython_DISABLE_CRS TRUE)

set (AutoSaveState_FORCE_SERIAL TRUE) # since this modifies settings

list(APPEND TESTS_WITH_INLINE_COMPARES
  RestoreArrayDefaultTransferFunction.xml
  RestoreDefaultTransferFunction.xml
  SelectCellsTrace.xml
  SelectionLinkScripting.xml
  SelectPointsTrace.xml
  SplitViewTrace.xml
  TraceMultiViews.xml
  )
set(SelectCellsTrace_DISABLE_CRS TRUE)
set(SelectPointsTrace_DISABLE_CRS TRUE)

# Check that matplotlib is available
include(ParaViewFindPythonModules)
find_python_module(matplotlib matplotlib_found)
if (matplotlib_found)
  list(APPEND TESTS_WITH_BASELINES TestPythonView.xml)
  list(APPEND TESTS_WITH_INLINE_COMPARES TextSourceInteriorLines.xml)
  list(APPEND TESTS_WITH_INLINE_COMPARES TextSourcesInChartViews.xml)
  list(APPEND TESTS_WITH_INLINE_COMPARES MathTextColumn.xml)
endif()

find_python_module(numpy numpy_found)
if (numpy_found)
  list(APPEND TESTS_WITH_BASELINES
    AnnotateNotSanitizedArray.xml
    ContextViewSelectionTrace.xml
    FindDataNameSanitization.xml
    FindDataNonDistributedData.xml
    FindDataPartialArrays.xml
    FindDataSelectLocationMultiblock.xml # find data needs python/numpy
    FindDataSelectLocation.xml # find data needs python/numpy
    FindDataSelectors.xml
    FindDataTime.xml
    ForceTimeDiamond.xml
    FreezeQueryMultiblock.xml
    IndexedLookupInitialization.xml # needs Python Calculator/numpy
    PlotOverLine_surface.xml # needs find data
    ProgrammableAnnotation.xml
    SpreadSheetSelectionTrace.xml
    StructuredGridCellBlanking.xml
    VolumeNoMapScalars.xml # needs programmable filter + numpy
    )

  list(APPEND TESTS_WITHOUT_BASELINES
    ExpressionChooser.xml
    ExpressionClear.xml
    FieldDataDomainDefault.xml
    HyperTreeGridGenerateFields.xml
    HTGPlotSelectionOverTime.xml
    PlotOverLine_htg.xml # needs find data
    ProgrammableFilterFieldData.xml
    PythonCalculator.xml
    PythonCalculatorArrayAssociation.xml
    PythonCalculatorAutocomplete.xml
    PythonCalculatorCrossComposite.xml
    PythonCalculatorFieldData.xml
    PythonCalculatorInput.xml
    PythonCalculatorMultiline.xml
    SelectionAndAutoSaveState.xml
    )

  set(SpreadSheetSelectionTrace_DISABLE_CS TRUE)
  set(SpreadSheetSelectionTrace_DISABLE_CRS TRUE)

  list(APPEND TESTS_WITH_INLINE_COMPARES
    ExodusModeShapes.xml
    FindDataTrace.xml
    FindDataQueries.xml
    TestTableFFT.xml # needs programmable filter + numpy
    )

  # PythonAlgorithm plugin tests.
  configure_file(
    "PythonAlgorithmPlugin.xml.in"
    "${CMAKE_CURRENT_BINARY_DIR}/PythonAlgorithmPlugin.xml" @ONLY)
  configure_file(
    "PythonAlgorithmReadersAndWriters.xml.in"
    "${CMAKE_CURRENT_BINARY_DIR}/PythonAlgorithmReadersAndWriters.xml" @ONLY)

  set(pyalgo_plugin_tests
    PythonAlgorithmPlugin
    PythonAlgorithmReadersAndWriters)
  foreach(tname IN LISTS pyalgo_plugin_tests)
    list(APPEND TESTS_WITH_BASELINES
      ${CMAKE_CURRENT_BINARY_DIR}/${tname}.xml)

    # we need to extend testing infrastructure to better support
    # loading plugins in client-server. At that point, we can test these as well.
    set(${tname}_DISABLE_CS TRUE)
    set(${tname}_DISABLE_CRS TRUE)
  endforeach()
endif()

#----------------------------------------------------------------------
# Tests that produce some output (other than rendered images) from ParaView
# that should be checked for correctness. These work by executing an XML
# test in the UI that produces some output, then pvpython runs a Python script
# that checks the file for correctness. Example use: checking a CSV
# file for expected content.
#
# Tests listed here are required to define the following files:
# - <test>.xml - XML test script to play in the uI
# - <test>Verify.py - Python code to verify output saved out in the XML
#   script
#----------------------------------------------------------------------
set(paraview_python_verify_tests
  ExportMultiblockFieldDataSpreadsheet
  ExportSceneSpreadSheetView
)

if (PARAVIEW_ENABLE_WEB)
  list(APPEND paraview_python_verify_tests
    AnimatedExportScene
  )
endif()

if (PARAVIEW_USE_PYTHON)
  foreach(test_name ${paraview_python_verify_tests})
    set(tname "ParaView::Applications::${test_name}")
    add_test(NAME ${tname}
      COMMAND ${CMAKE_COMMAND}
      -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::paraview>
      -DPVPYTHON_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvpython>
      -DDATA_DIR:PATH=${paraview_test_data_directory_output}
      -DTEST_NAME:STRING=${test_name}
      -DTEST_SCRIPT:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/${test_name}.xml
      -DTEST_VERIFIER:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/${test_name}Verify.py
      -DTEMPORARY_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
      -P ${CMAKE_CURRENT_SOURCE_DIR}/PythonScriptTestDriver.cmake
    )
    set_tests_properties(${tname} PROPERTIES LABELS "paraview")
  endforeach()
endif()
