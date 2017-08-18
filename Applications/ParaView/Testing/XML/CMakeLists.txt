# TODO: Split these up and put them in the conditional which actually requires
# the data.
paraview_test_load_data_dirs(""
  "Iron Xdmf"
  amr
  ANALYZE
  CompositeGlyphTree
  dualSphereAnimation
  ensemble-wavelet
  EnSight
  ExRestarts
  FieldDataBlocks
  FieldDataBlocks/FieldDataBlocks
  FileSeries
  GMV
  GMV/invalid
  GMV/partial-pyramid
  GMV/quadratic_cells
  RodPlate
  singleSphereAnimation
  SLAC/pic-example
  SPCTH
  SPCTH/Dave_Karelitz_Small
  SPCTH/DerivedDensity
  VisItBridge
  )

paraview_test_load_data(""
  ,REGEX:tensors_.*\\.vti
  artifact.dta
  "blow data.myvtk"
  blow.vtk
  bluntfin.vts
  can.ex2
  comb.q
  combxyz.bin
  CompositeGlyphInput.vtp
  CompositeGlyphTree.vtm
  CubeStringArray.vtk
  cylinder_run1.e
  cylinder_run2.e
  disk_out_ref.ex2
  dualSphereAnimation4.pvd
  dualSphereAnimation.clone.pvd
  dualSphereAnimation.pvd
  "iron protein.vtk"
  LoadState.pvsm
  LoadStateDialog.pvsm
  minimal.nii
  MultiBlockWithPieces_0_0.vtp
  MultiBlockWithPieces_1_0.vtp
  MultiBlockWithPieces.vtm
  multicomb_0.vts
  multicomb_1.vts
  multicomb_2.vts
  NE2_ps_bath.png
  non_convex_polygon.vtu
  office.binary.vtk
  porphyrin.cml
  PropertyLink.pvsm
  quadraticTetra01.vtu
  RectGrid2.vtk
  sample.h5part
  Scenario1_p1.h5
  Scenario1_p1.xmf
  servers-wiki.pvsc
  sineWaves.csv
  singleSphereAnimation.pvd
  tensors.pvti
  Test1.h5
  Test1.xmf
  TestMultipleNumberOfComponentsPlugin.xml
  tube.exo
  vehicle_data.csv
  viscoplastic-ring.h5
  )

SET (TESTS_WITHOUT_BASELINES
  ${CMAKE_CURRENT_SOURCE_DIR}/AboutDialog.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ArrayBoundsScaleDomain.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/DelimitedTextReader.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExportFilteredColumnsSpreadsheet.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/FindWidget.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/OpenHelp.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ServerConnectDialog.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SearchBox.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/TableHistogram.xml
)

if (PARAVIEW_ENABLE_PYTHON)
  list(APPEND TESTS_WITHOUT_BASELINES
    ${CMAKE_CURRENT_SOURCE_DIR}/DetachedFromLayoutIndirect.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/TestPythonConsole.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/TestPopOutWidget.xml # needs programmable filter
    ${CMAKE_CURRENT_SOURCE_DIR}/MultipleNumberOfComponents.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/SpreadSheetNullArrayName.xml # needs programmable filter
  )
endif()

SET (TESTS_WITH_INLINE_COMPARES
  ${CMAKE_CURRENT_SOURCE_DIR}/AxesGrid.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CalculatorQuotedVariable.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CameraLink.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ChartLoadNoVariables.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ClampAndUpdateColorMap.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Contour.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/DisconnectAndSaveAnimation.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Ensemble.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/FileSeries.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/LineChartSelection.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/LoadStateMultiView.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/LogColorMap.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/MemoryInspectorPanel.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Molecule.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/NonlinearSubdivisionDisplay.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/QuartilePlot.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/RectilinearFractal.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ReloadExodusFile.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/RemoteRendering.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/RenderNan.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ResetToVisibleRange.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Ruler.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SaveLargeScreenshot.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/TestIsoVolume.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/TransferFunctionResetOnVisibilityChange.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/UpdateTransferFunctionRanges.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/XYChart.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/XYHistogram.xml
)

if(NOT APPLE)
  # Workaround to avoid testing failures on high-res Retina displays on Macs
  list(APPEND TESTS_WITH_INLINE_COMPARES
    ${CMAKE_CURRENT_SOURCE_DIR}/ColorLegendScaledScreenshot.xml
  )
endif()

SET(TESTS_WITH_BASELINES
  ${CMAKE_CURRENT_SOURCE_DIR}/MoleculeScalarBar.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/TestDataGridAxes.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/TestHiddenLineRemoval.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/TestOpacityRendering.xml
)

if(PARAVIEW_USE_VISITBRIDGE)
  list(APPEND TESTS_WITH_BASELINES
    ${CMAKE_CURRENT_SOURCE_DIR}/VisItBridgeChombo.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/VisItBridgeEnzo.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/VisItBridgeLAMMPSDump.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/VisItBridgeLAMMPSDump2.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/VisItBridgeNas.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/VisItBridgeNek.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/VisItBridgePdb.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/VisItBridgePFLOTRAN.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/VisItBridgePixie.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/VisItBridgeSamrai.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/TruchasReaderWithVisItBridge.xml)
else()
  # The TruchasReader test behavior changes depending on whether the
  # VisItBridge is enabled. A number of VisIt readers support
  # reading of .h5 files, so when the VisItBridge is enabled,
  # the test must take extra steps to select the correct reader.
  # When the VisItBridge is not enabled, there is no need to select
  # a reader.
  list(APPEND TESTS_WITH_BASELINES
    ${CMAKE_CURRENT_SOURCE_DIR}/TruchasReader.xml)
endif()

if(PARAVIEW_USE_OSPRAY)
  list(APPEND TESTS_WITH_BASELINES
    ${CMAKE_CURRENT_SOURCE_DIR}/OSPRay.xml)
endif()

# hits OSX bug with FileDialog so exclude test on APPLE
if (NOT APPLE)
  list (APPEND TESTS_WITH_INLINE_COMPARES
    ${CMAKE_CURRENT_SOURCE_DIR}/HistogramSelection.xml)
endif()

# Catalyst-Live tests
# ----------------------------------------------------------------------
if(PARAVIEW_ENABLE_CATALYST AND PARAVIEW_ENABLE_PYTHON)
  set (catalyst_live_tests
    CatalystLiveSetBreakpoint
    CatalystLivePause)
  set (CATALYST_SIMULATION CatalystWaveletDriver.py)
  set (INSITU_SCRIPT CatalystWaveletCoprocessing)
  # Test-specific parameters
  set (CatalystLiveSetBreakpoint_DURATION 222)
  set (CatalystLivePause_DURATION 80)

  foreach (tname ${catalyst_live_tests})
    # These tests are too complex to run in parallel.
    set ("${tname}_FORCE_SERIAL" TRUE)
    add_pv_test("pv" "_DISABLE_C"
      COMMAND
      --script-ignore-output-errors --script $<TARGET_FILE:pvbatch> -sym
      ${CMAKE_CURRENT_SOURCE_DIR}/${CATALYST_SIMULATION}
      ${INSITU_SCRIPT} "${${tname}_DURATION}"

      --client ${CLIENT_EXECUTABLE}
      --enable-bt
      -dr
      --test-directory=${PARAVIEW_TEST_DIR}
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/${tname}.xml
      EXTRA_LABELS "CATALYST")

    add_pv_test(pvcs "_DISABLE_CS"
      COMMAND
      --script-np 5 --script-ignore-output-errors
      --script $<TARGET_FILE:pvbatch> -sym
      ${CMAKE_CURRENT_SOURCE_DIR}/${CATALYST_SIMULATION}
      ${INSITU_SCRIPT} "${${tname}_DURATION}"

      --server $<TARGET_FILE:pvserver>
      --enable-bt

      --client ${CLIENT_EXECUTABLE}
      --enable-bt
      -dr
      --test-directory=${PARAVIEW_TEST_DIR}
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/${tname}.xml
      EXTRA_LABELS "CATALYST")
  endforeach()
endif()

if (NOT PARAVIEW_ENABLE_XDMF3)
  # Presently, all these tests only work with Xdmf2 and when Xdmf3 is enabled,
  # the SelectReader dialog confuses these tests. So, we'll disable them for now.
  # We need a better mechanism to handle such cases in the testing framework.
  list(APPEND TESTS_WITH_BASELINES
    ${CMAKE_CURRENT_SOURCE_DIR}/ExtractBlock.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/XdmfRead.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/XdmfReadImageData.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/XdmfReadImageDataCollection.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/XdmfReadRectilinearGrid.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/XdmfReadRectilinearGridCollection.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/XdmfReadStructuredGrid.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/XdmfReadStructuredGridCollection.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/XdmfGridAttributes.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/ZLibXDMF.xml
    )
endif()

list(APPEND TESTS_WITH_BASELINES
  ${CMAKE_CURRENT_SOURCE_DIR}/AccentuatedFilename.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Annotations.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CopyPasteProperties.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CTHAMRBaseline.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CTHAMRClip.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CTHAMRContour.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CTHAMRDualClip.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CTHAMRMaterialInterfaceFilter.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CTHDerivedDensity2DClyinder.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/AddActiveValuesFromVisibleObjects.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/AnimatePipelineTime.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/AngularPeriodicFilter.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/BagPlots.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/3DWidgetInCustomFilter.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CalcParens.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Calculator.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ChangingTimestepsInStateFiles.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ChartAxisRangeAndLabels.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CheckableHeader.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Clip.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ColorEditorControls.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ColorEditorVolumeControls.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ColorLegendMinMaxLabels.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ColorOpacityTableEditing.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CategoricalColors.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ColorByCellDataStringArray.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ComparativeVisPanel.xml
  # disabling overlay test for now. It has issues with Time 0.
  #  ${CMAKE_CURRENT_SOURCE_DIR}/ComparativeOverlay.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CompositeSurfaceSelection.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CreateDelete.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CustomFilter.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CustomSourceProbe.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/DataOutlines.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/DualSphereAnimation.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/EnSight.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/EnSightTensorInversion.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/EnSightTensorInversionBin.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExodusXML.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExportLinePlotToCSV.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExportX3dPOVVRML.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExTimeseries.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExTimeseries2.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExtractComponentFilter.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExtractLevel.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExtractTimeSteps.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/FFTOverTime.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/FieldDataBlocks.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Flow.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Flow2.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/FollowHiddenData.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ForceTime.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Fractal2D.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/FunctionalBagPlots.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Glyph3DRepresentation.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/GlyphUseCellCenters.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/H5PartReader.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/HideAll.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ImageVolumeRendering.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/InteractiveSelection.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/InteractiveViewLinkOffscreen.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/InteractiveViewLinkOnscreen.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/InteractiveViewLinkState.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/LoadPartiallyUpdatedStateFiles.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/LoadSaveStateAnimation.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/LoadState.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/LoadStateChooseDirectory.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/LoadStateChooseFiles.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/LoadStateUseExisting.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/LoadSaveStateVolume.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/MultiBlockAttributes1.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/MultiBlockInspector.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/MultiBlockSingleBlock.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/MultiSliceMultiBlock.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/MultiSliceWavelet.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/NormalGlyphs.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/NumberOfComponentsDomain.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/NiftiReaderWriterPlugin.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/NonConvexPolygon.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/OpenSaveData.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/OrthographicView.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/OutOfRangeColors.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Plot3DReader.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PlotEdges2.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PlotEdges.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PlotGlobalVariablesOverTime.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PlotOverTimeAutoApply.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PolygonCellSelection.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PolygonPointSelection.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PointInterpolator.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PolarAxes.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PropertyConversion.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PropertyConversion1.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PropertyConversion2.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PropertyConversion3.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PropertyLink.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/QuadraticGhostCells.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ReadXMLPolyDataFileSeries.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/RepresentationSelector.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ResampleWithDataset.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/RescaleToTemporal.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/RotateCamera90.xml
#  ${CMAKE_CURRENT_SOURCE_DIR}/SaveAnimationGeometry.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SaveColorMap.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SaveCSV.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SelectedProxyPanelVisibility.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SelectionModifiersCells.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SelectionModifiersPoints.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SelectionModifiersBlocks.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SelectReader.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SimpleInteraction.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Slice.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SliceAlongPolyline.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SliceAlongSpline.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SolidColorSource.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SphereSlice.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SpherePointSource.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SpreadSheet1.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SpreadSheet2.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SpreadSheet3.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SPTimeseries.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SpyPlotHistoryReader.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SelectionLinkBasic.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SelectionLinkConverted.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SelectionLinkHistogram.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SelectionLinkInitial.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SelectionLinkMultiple.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SelectionLinkRemove.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SelectionLinkReaction.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SelectionLinkReactionNonConverted.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SortLineChartData.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/StreamTracerSurface.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/StockColors.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/StreamTracerUpdates.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/TabbedViews.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/TensorGlyph.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/TemporalInterpolator.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/TemporalShiftScale.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/TestGroupDataFromTimeSeries.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/TestSelectionOnMultipiece.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/TimeInspector.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/TimeStepProgressBar.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Tessellate.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Threshold.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/UndoRedo.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/UndoRedo1.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/UndoRedo2.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/UndoRedo3.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/UndoRedo5.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/UndoRedo6.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/UndoRedo7.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/UndoRedo8.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/UnstructuredOutline.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/VolumeRenderingWithContour.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/VariableSelector.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/VariableSelector1.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ViewSettingsDialog.xml
  #  ${CMAKE_CURRENT_SOURCE_DIR}/XYBarChart.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/RandomAttributes.xml
)

if(VTK_USE_LARGE_DATA)
  paraview_test_load_data_dirs(""
    bake
  )
  list(APPEND TESTS_WITH_BASELINES
    ${CMAKE_CURRENT_SOURCE_DIR}/CheckSelectedBlocks.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/RestoreBlockColorDefaultTransferFunction.xml
    )
endif()

# The color dialog is different on MacOS X which makes this test fail.
if(NOT APPLE)
  list(APPEND TESTS_WITH_BASELINES
    ${CMAKE_CURRENT_SOURCE_DIR}/BackgroundLights.xml
    )
endif()

if("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  list(APPEND TESTS_WITH_BASELINES
    # This test makes use of "Point Gaussian" representation, only available with OpenGL2
    ${CMAKE_CURRENT_SOURCE_DIR}/BackgroundColorCheck.xml

    # PlotMatrixView is bugging in OpenGL1
    ${CMAKE_CURRENT_SOURCE_DIR}/PlotMatrixViewArraySelection.xml

    # Composite Glyph Trees are only implemented in OpenGL2
    ${CMAKE_CURRENT_SOURCE_DIR}/CompositeGlyphTree.xml
    )
endif()

if (PARAVIEW_USE_MPI)
  list(APPEND TESTS_WITH_BASELINES
    # Test uses D3 which is available in MPI only builds.
    ${CMAKE_CURRENT_SOURCE_DIR}/D3SmallCells.xml

    # Test uses D3 which is available in MPI only builds.
    ${CMAKE_CURRENT_SOURCE_DIR}/GridConnectivity.xml

    # Test uses D3 which is available in MPI only builds.
    ${CMAKE_CURRENT_SOURCE_DIR}/ExportSelectionToCSV.xml

    # Test Use Data Partitions for volume rendering
    #${CMAKE_CURRENT_SOURCE_DIR}/UseDataPartitions.xml
  )

  # This test requires a parallel partition to make sense so we
  # skip running it with the built-in server.
  add_client_server_tests("pvcs"
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/UseDataPartitions.xml
  )


  # Test AppendReduce which can only be tested with non-built-in mode
  add_client_server_tests("pvcs"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/AppendReduce.xml
  )
  # Test AppendReduce which can only be tested with non-built-in mode
  add_client_server_tests("pvcrs"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/AppendReduce.xml
  )

  # Test uses GhostCellsGenerator which is available in MPI only builds.
  # As the test use pvtu writer/reader it does not work in built-in mode.
  add_client_server_tests("pvcs"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/GhostCellsGenerator.xml
    )
  add_client_render_server_tests("pvcrs"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/GhostCellsGenerator.xml
    )

  if (PARAVIEW_ENABLE_COSMOTOOLS)
    # Test the Generic IO file writer in VTKExtensions/CosmoTools
    add_client_tests("pv"
      --mpi
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/GenericIOReadWrite.xml
      )
    add_client_server_tests("pvcs"
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/GenericIOReadWrite.xml
      )
    add_client_render_server_tests("pvcrs"
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/GenericIOReadWrite.xml
      )

  endif()
  list(APPEND TESTS_WITH_BASELINES
    ${CMAKE_CURRENT_SOURCE_DIR}/CGNSReader-5blocks_cgns.xml)

  # PointInterpolator only works in non-distributed modes.
  set(PointInterpolator_DISABLE_CRS TRUE)
  set(PointInterpolator_DISABLE_CS TRUE)
endif()
if(PARAVIEW_ENABLE_PYTHON)
  list(APPEND TESTS_WITH_BASELINES
    ${CMAKE_CURRENT_SOURCE_DIR}/ColorByComponentNames.xml # needs programmable filter
    ${CMAKE_CURRENT_SOURCE_DIR}/SelectionLabels.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/TraceExodus.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/TraceExportAndSaveData.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/TraceSaveGeometry.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/TraceSupplementalProxiesFully.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/SaveLoadStatePython.xml
  )
  list(APPEND TESTS_WITH_INLINE_COMPARES
    ${CMAKE_CURRENT_SOURCE_DIR}/RestoreArrayDefaultTransferFunction.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/RestoreDefaultTransferFunction.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/TraceMultiViews.xml
  )
  list(APPEND TESTS_WITHOUT_BASELINES
    ${CMAKE_CURRENT_SOURCE_DIR}/PythonCalculator.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/TraceIntegrateVariables.xml
  )

  # Check that matplotlib is available
  include(FindPythonModules)
  find_python_module(matplotlib matplotlib_found)
  if (PARAVIEW_ENABLE_MATPLOTLIB AND matplotlib_found)
    list(APPEND TESTS_WITH_BASELINES ${CMAKE_CURRENT_SOURCE_DIR}/TestPythonView.xml)
  endif()

  find_python_module(numpy numpy_found)
  if (numpy_found)
    list(APPEND TESTS_WITH_BASELINES
      ${CMAKE_CURRENT_SOURCE_DIR}/ExodusModeShapes.xml
      ${CMAKE_CURRENT_SOURCE_DIR}/ForceTimeDiamond.xml
      ${CMAKE_CURRENT_SOURCE_DIR}/StructuredGridCellBlanking.xml
      ${CMAKE_CURRENT_SOURCE_DIR}/TestFindDataMax.xml)
    list(APPEND TESTS_WITH_INLINE_COMPARES
      ${CMAKE_CURRENT_SOURCE_DIR}/FindDataDialog.xml)
  endif()

  if("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2" AND numpy_found)
    # Cinema importing capabilities are only available in Python + OpenGL2
    # add the cinema database files.
    paraview_test_load_data("" "cinema-composite.cdb/image/info.json")
    foreach(poseidx RANGE 0 17)
      foreach(visidx RANGE 0 1)
        paraview_test_load_data_dirs("" "cinema-composite.cdb/image/pose=${poseidx}/vis=${visidx}")
      endforeach()
    endforeach()
    list(APPEND TESTS_WITH_BASELINES
      ${CMAKE_CURRENT_SOURCE_DIR}/ImportCinema.xml)

    # presently, we don't support client-server configurations.
    set(ImportCinema_DISABLE_CS TRUE)
    set(ImportCinema_DISABLE_CRS TRUE)
  endif()
endif()

#------------------------------------------------------------------------------
# Add streaming tests.
# We need to locate smooth.flash since it's not included in the default testing
# datasets.
find_file(smooth_flash NAMES smooth.flash
          DOC "Path to smooth.flash data file."
          NO_DEFAULT_PATH)
mark_as_advanced(smooth_flash)
if (EXISTS "${smooth_flash}")
  # we configure the file since we need to point to smooth_flash.
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/AMRStreaming.xml.in"
                 "${CMAKE_CURRENT_BINARY_DIR}/AMRStreaming.xml" @ONLY)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/AMRVolumeRendering.xml.in"
                 "${CMAKE_CURRENT_BINARY_DIR}/AMRVolumeRendering.xml" @ONLY)

  set (streaming_tests
    ${CMAKE_CURRENT_BINARY_DIR}/AMRStreaming.xml)

  # AMRVolumeRendering is a non-streaming test.
  list(APPEND TESTS_WITH_BASELINES
    ${CMAKE_CURRENT_BINARY_DIR}/AMRVolumeRendering.xml)
  set(AMRVolumeRendering_BREAK TRUE)

  foreach (tname ${streaming_tests})
    add_pv_test("pv" "_DISABLE_C"
      COMMAND --client ${CLIENT_EXECUTABLE}
              --enable-bt
              -dr
              --enable-streaming
              --test-directory=${PARAVIEW_TEST_DIR}

      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${tname})

    add_pv_test(pvcs "_DISABLE_CS"
      COMMAND
         --server $<TARGET_FILE:pvserver>
         --enable-bt
         --enable-streaming
         --client ${CLIENT_EXECUTABLE}
         --enable-bt
         -dr
         --enable-streaming
         --test-directory=${PARAVIEW_TEST_DIR}
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${tname})
  endforeach()
endif()

#------------------------------------------------------------------------------

# Probe picking does not work in render server mode

#------------------------------------------------------------------------------
set (TestPythonConsole_BREAK TRUE)

# Selection is not available in CRS mode
set(MultiSliceWavelet_DISABLE_CRS TRUE)
set(MultiSliceMultiBlock_DISABLE_CRS TRUE)
set(PolygonCellSelection_DISABLE_CRS TRUE)
set(PolygonPointSelection_DISABLE_CRS TRUE)
set(NonlinearSubdivisionDisplay_DISABLE_CRS TRUE)
set(InteractiveSelection_DISABLE_CRS TRUE)
set(NonConvexPolygon_DISABLE_CRS TRUE)
set(QuartilePlot_DISABLE_CRS TRUE)
set(TestSelectionOnMultipiece_DISABLE_CRS TRUE)
set(CheckSelectedBlocks_DISABLE_CRS TRUE)
set(SelectionModifiersCells_DISABLE_CRS TRUE)
set(SelectionModifiersBlocks_DISABLE_CRS TRUE)
set(SelectionModifiersPoints_DISABLE_CRS TRUE)
set(SelectionLinkBasic_DISABLE_CRS TRUE)
set(SelectionLinkInitial_DISABLE_CRS TRUE)
set(SelectionLinkMultiple_DISABLE_CRS TRUE)
set(SelectionLinkRemove_DISABLE_CRS TRUE)
set(SelectionLinkReaction_DISABLE_CRS TRUE)
set(ExportSelectionToCSV_DISABLE_CRS TRUE)

# These tests cannot be run using ctest -j since they are affected by focus
# changed events.
set (AnimatePipelineTime_FORCE_SERIAL TRUE)
set (PolygonCellSelection_FORCE_SERIAL TRUE)
set (PolygonPointSelection_FORCE_SERIAL TRUE)
set (Contour_FORCE_SERIAL TRUE) # since this uses popup-menu
set (Calculator_FORCE_SERIAL TRUE) # since this uses popup-menu
set (LogColorMap_FORCE_SERIAL TRUE) # since this uses popup-menu
set (ColorOpacityTableEditing_FORCE_SERIAL TRUE) # since this uses popup-menu
set (SimpleInteraction_FORCE_SERIAL TRUE) # since this uses popup-menu
set (FindWidget_FORCE_SERIAL TRUE) # Uses inline edit popups
set (PropertyLink_FORCE_SERIAL TRUE)
set (TestPythonView_FORCE_SERIAL TRUE) # Seems to work better in serial.
set (Contour_FORCE_SERIAL TRUE) # Uses inline edit popups
set (SaveColorMap_FORCE_SERIAL TRUE) # Uses inline edit popups
set (CategoricalColors_FORCE_SERIAL TRUE) # Uses inline edit popups
set (ChartAxisRangeAndLabels_FORCE_SERIAL TRUE) # Uses inline edit popups

set (LoadSaveStateVolume_FORCE_SERIAL TRUE)
# We don't support volume rendering of image data in data server & render server mode
set(LoadSaveStateVolume_DISABLE_CRS TRUE)

# Set properties for CTH tests
set(CTHAMRContour_DISABLE_CS TRUE)
set(CTHAMRContour_DISABLE_CRS TRUE)
set(CTHAMRDualClip_DISABLE_CS TRUE)
set(CTHAMRDualClip_DISABLE_CRS TRUE)
set(CTHAMRClip_DISABLE_CS TRUE)
set(CTHAMRClip_DISABLE_CRS TRUE)
set(CTHAMRMaterialInterfaceFilter_DISABLE_CS TRUE)
set(CTHAMRMaterialInterfaceFilter_DISABLE_CRS TRUE)

# Since this test needs selection, disable in CRS mode.
set (SelectionLabels_DISABLE_CRS TRUE)

# Mark tests that change change state considerably requires a restart of the
# test suite when grouping tests together for faster playback.
SET (UndoRedo1_BREAK TRUE)
SET (UndoRedo2_BREAK TRUE)
SET (UndoRedo7_BREAK TRUE)
  # <-- since these are just long tests, they result in timeouts.

# Add image threshold overrides for tests.
# Generally all tests with wireframes need higher thresholds.
SET (Flow2_THRESHOLD 15)
SET (UndoRedo1_THRESHOLD 15)
SET (UndoRedo4_THRESHOLD 15)

# Differnces between systems on whether the lines are anti-aliased or not
SET (SortLineChartData_THRESHOLD 50)

# Since often there are issues with fonts etc.
SET (XYChart_THRESHOLD 150)
SET (CheckableHeader_THRESHOLD 150)
# Since the test uses surface-selection, it cannot work in render-server mode.
set(XYChart_DISABLE_CRS TRUE)

SET (XYHistogram_THRESHOLD 150)
set (ChartAxisRangeAndLabels_THRESHOLD 60)

# The color and opacity are often different on different machines.
SET (ColorEditorControls_THRESHOLD 60)
SET (ColorEditorVolumeControls_THRESHOLD 300)

# Plot image leeway.
set (SpreadSheet2_THRESHOLD 50)
set (PlotOverTimeAutoApply_THRESHOLD 50)
set (ExportLinePlotToCSV_THRESHOLD 30)
set (PlotOverTimeAutoApply_DISABLE_CRS TRUE) # since this uses surface selection.
set (QuartilePlot_THRESHOLD 20)


# DisconnectAndSaveAnimation is only support in Client-Server mode.
set (DisconnectAndSaveAnimation_DISABLE_C TRUE)
set (DisconnectAndSaveAnimation_DISABLE_CRS TRUE)
set (DisconnectAndSaveAnimation_BREAK TRUE)

# Cannot support CRS since we do volume rendering of image data
# in this test.
set (RemoteRendering_DISABLE_CRS TRUE)
set (RemoteRendering_THRESHOLD 50)
set (VolumeRenderingWithContour_DISABLE_CRS 50)
set (VolumeRenderingWithContour_THRESHOLD 50)

# Histogram filter produces different results when running in parallel, so
# disable it.
IF (PARAVIEW_USE_MPI)
  SET (SpreadSheet1_DISABLE_CS TRUE)
  SET (SpreadSheet1_DISABLE_CRS TRUE)

  # Disabled since the 1 column in spreadsheet view ends up being "Process ID"
  # which messes up the sorting in this test. Need to extend the testing framework
  # or fix spreadsheet view for these to work properly.
  SET (SpreadSheet3_DISABLE_CS TRUE)
  SET (SpreadSheet3_DISABLE_CRS TRUE)

  # The hierchical fractal source is a temporary testing source and it does not
  # create the dataset correctly in parallel. Since it's a testing source, I am
  # just going to disable the test in parallel. We can fix the source when
  # needed.
  SET (RectilinearFractal_DISABLE_CS TRUE)
  SET (RectilinearFractal_DISABLE_CRS TRUE)

  # Selections end up highlighting different set of ID based points in parallel.
  # Hence disable them.
  set (LineChartSelection_DISABLE_CS TRUE)
  set (LineChartSelection_DISABLE_CRS TRUE)

  # Selections end up highlighting different set of ID based points in parallel.
  # Hence disable them.
  set (BagPlots_DISABLE_CS TRUE)
  set (BagPlots_DISABLE_CRS TRUE)
  set (FunctionalBagPlots_DISABLE_CS TRUE)
  set (FunctionalBagPlots_DISABLE_CRS TRUE)

  # Selection link may highlight incorrect ID in parallel,
  # hence disable them.
  set(SelectionLinkBasic_DISABLE_CS TRUE)
  set(SelectionLinkInitial_DISABLE_CS TRUE)
  set(SelectionLinkMultiple_DISABLE_CS TRUE)
  set(SelectionLinkRemove_DISABLE_CS TRUE)
  set(SelectionLinkReaction_DISABLE_CS TRUE)
ENDIF ()

# Composite Surface Selection is currently broken in everything but bultin
SET (CompositeSurfaceSelection_DISABLE_CS TRUE)
SET (CompositeSurfaceSelection_DISABLE_CRS TRUE)

# Clip test has object picking which is not supported in client-render-server
# mode.
SET (Clip_DISABLE_CRS TRUE)

# Image volume rendering not supported in CRS mode.
set (ImageVolumeRendering_DISABLE_CRS TRUE)

# These Xdmf tests have wireframes, hence the increased thresholds.
SET (XdmfReadImageData_THRESHOLD 20)
SET (XdmfReadRectilinearGrid_THRESHOLD 20)
SET (XdmfReadRectilinearGridCollection_THRESHOLD 20)
SET (XdmfReadStructuredGrid_THRESHOLD 20)
SET (XdmfReadStructuredGridCollection_THRESHOLD 20)
SET (XdmfReadImageDataCollection_THRESHOLD 20)

# This test renders points and they are offsetted a bit differently on
# different platforms so just increase the threshold
SET (SaveCSV_THRESHOLD 40)

# Disable some testing configurations for these tests.

## Disable ClientRenderServer tests for FFTOverTime. This is done since
## selection is not supported in render server mode esp. when number of render
## server processess is not same as the data server processes
set (FFTOverTime_DISABLE_CRS TRUE)
set (FFTOverTime_THRESHOLD 70) # since there's a plot, account for minor
                               # rendering differences.

# ColorEditorVolumeControls does volume rendering of structrued data which required remote
# rendering in client-serve mode.
SET (ColorEditorVolumeControls_DISABLE_CS TRUE)
SET (ColorEditorVolumeControls_DISABLE_CRS TRUE)

# ViewSettingsDialog uses texture background which is currently supported only
# in local render mode.
SET (ViewSettingsDialog_DISABLE_CS TRUE)
SET (ViewSettingsDialog_DISABLE_CRS TRUE)

# MultiBlockAttributes1 requires selection which doesn't work on pvcrs
SET (MultiBlockAttributes1_DISABLE_CRS TRUE)

# Plugins are only built as shared libraries.
IF (NOT BUILD_SHARED_LIBS)
  SET (NiftiReaderWriterPlugin_DISABLE_C TRUE)
ENDIF ()
# There should be a client server specific version of this test.
SET (NiftiReaderWriterPlugin_DISABLE_CS TRUE)
SET (NiftiReaderWriterPlugin_DISABLE_CRS TRUE)

# Set image threshold overrides for the tests.
SET(SaveLargeScreenshot_THRESHOLD 40)

# Make these tests use reverse connection.
SET (CutMulti_REVERSE_CONNECT TRUE)

# Since this involves charts.
SET(HistogramSelection_THRESHOLD 40)
SET(LineChartSelection_THRESHOLD 40)
SET(FunctionalBagPlots_THRESHOLD 40)

# Increate threshold for this one since the chart axes
# tends to render slightly differently.
set (LoadStateMultiView_THRESHOLD 20)

# Selection not supported in CRS and is needed for this test.
set (ResetToVisibleRange_DISABLE_CRS TRUE)

# Molecules are not supported in client-serve modes, currently.
set (Molecule_DISABLE_CS TRUE)
set (Molecule_DISABLE_CRS TRUE)

##########################################################
# List of known incompatible tests for Collaboration
#
#   Contains Undo/Redo actions:
#     - Clip, UndoRedo, UndoRedo[1-8]
#
#   Contains Plugin Loading actions:
#     - LoadPlugins, NiftiReaderWriterPlugin, H5PartReaderPlugin
#
#   Does not work in CS or CRS mode
#     - ViewSettingsDialog, RectilinearFractal, NewColorEditor
#
#   Does not work because a disconnection is involved
#     - ExportX3dPOVVRML
#
#   Don't want to support for now
#     - ComparativeVisPanel
#     - LoadSaveStateAnimation: The time get reset by collab
#
#   Timing issues (Works on speedy computer)
#     - 3DWidgetInCustomFilter, CustomFilter
#
##########################################################

OPTION(PARAVIEW_COLLABORATION_TESTING
    "Add Collaboration testing"
    ON)
MARK_AS_ADVANCED(PARAVIEW_COLLABORATION_TESTING)

# The create/delete test was only meant for collaboration
SET (CreateDelete_DISABLE_C TRUE)
SET (CreateDelete_DISABLE_CS TRUE)
SET (CreateDelete_DISABLE_CRS TRUE)

# List of test Enable for collaboration
#set (AnimatePipelineTime_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (CalcParens_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (Calculator_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (CameraLink_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (CompositeSurfaceSelection_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
set (Contour_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (ContourRange_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
set (CreateDelete_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (CustomSourceProbe_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (D3SmallCells_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (DualSphereAnimation_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (EnSight_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (ExodusModeShapes_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (ExodusXML_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (ExTimeseries_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (ExTimeseries2_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (ExtractBlock_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (ExtractLevel_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (FindDataDialog_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (FFTOverTime_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (Flow_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (Flow2_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (Fractal2D_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (GridConnectivity_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (NormalGlyphs_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (OpenSaveData_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (Plot3DReader_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (PlotEdges_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (PlotEdges2_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (PropertyConversion_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
set (PropertyConversion1_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
set (PropertyConversion2_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
## disabling since the state file doesn't have view layout and such old state
## files are not currently supported in collaborative mode.
##set (PropertyLink_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (ReadXMLPolyDataFileSeries_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (RepresentationSelector_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (SaveColorMap_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (SaveCSV_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (SelectReader_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (SimpleInteraction_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (Slice_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
set (SpreadSheet1_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (SpreadSheet2_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (SPTimeseries_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (SpyPlotHistoryReader_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (StreamTracerUpdates_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (TemporalInterpolator_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (TemporalShiftScale_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (Tessellate_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (Threshold_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (UnstructuredOutline_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
set (VariableSelector_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
set (VariableSelector1_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
set (XdmfRead_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (XdmfReadImageData_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (XdmfReadImageDataCollection_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (XdmfReadRectilinearGrid_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (XdmfReadRectilinearGridCollection_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (XdmfReadStructuredGrid_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (XdmfReadStructuredGridCollection_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (XdmfGridAttributes_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
set (ZLibXDMF_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})


set(TESTS_WITH_MULTI_SERVERS_3
  ${CMAKE_CURRENT_SOURCE_DIR}/TestMultiServer3.xml
)

list(APPEND TESTS_WITHOUT_BASELINES
  ${TESTS_WITH_INLINE_COMPARES})

add_client_tests("pv"
  TEST_SCRIPTS ${TESTS_WITHOUT_BASELINES}
  )

add_client_tests("pv"
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS ${TESTS_WITH_BASELINES}
)

add_client_server_tests("pvcs"
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS ${TESTS_WITH_BASELINES}
)

add_client_server_tests("pvcs"
  TEST_SCRIPTS ${TESTS_WITH_INLINE_COMPARES}
)

IF (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
  add_client_render_server_tests("pvcrs"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${TESTS_WITH_BASELINES}
  )

  add_client_render_server_tests("pvcrs"
    TEST_SCRIPTS ${TESTS_WITH_INLINE_COMPARES}
  )
ENDIF ()

# TODO: remote rendering tests and reverse connect tests.

add_multi_client_tests("pvcs-collab"
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS ${TESTS_WITH_BASELINES})

add_multi_server_tests("pvcs-multi-servers" 3
   BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
   TEST_SCRIPTS ${TESTS_WITH_MULTI_SERVERS_3})

# Removing these test since these have been failing for a while the the testing
# infrastrructure not catching it properly until now. We need to redo the tile display
# testing :(.
#set (TileDisplayScatterPlot-2x1_THRESHOLD 40)
#add_tile_display_tests("pvcs-tile-display" 2 1
#   BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
#   TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/TileDisplayScatterPlot-2x1.xml)
#
#set (TileDisplayScatterPlot-2x2_THRESHOLD 40)
#add_tile_display_tests("pvcs-tile-display" 2 2
#   BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
#   TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/TileDisplayScatterPlot-2x2.xml)

# This test has been behaving badly across many platforms and needs to be fixed
# before it can be useful. Disabling for now, see bug #15424.
#set (TileDisplay3DTesting-1x2_THRESHOLD 270)
#add_tile_display_tests("pvcs-tile-display" 1 2
#   BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
#   TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/TileDisplay3DTesting-1x2.xml)

set (TileDisplay3DTesting-2x1_THRESHOLD 270)
add_tile_display_tests("pvcs-tile-display" 2 1
   BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
   TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/TileDisplay3DTesting-2x1.xml)

set (TileDisplay3DTesting-2x2_THRESHOLD 270)
add_tile_display_tests("pvcs-tile-display" 2 2
   BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
   TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/TileDisplay3DTesting-2x2.xml)


#------------------------------------------------------------------
# Add tests that test command line arguments (among other things).
#------------------------------------------------------------------
if (TARGET paraview)
  # The state file need to point to the correct data file. We do that by
  # configuring the state file.
  configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/exodusStateFile.3.14.1.pvsm.in"
    "${CMAKE_CURRENT_BINARY_DIR}/exodusStateFile.3.14.1.pvsm" @ONLY)
  add_pv_test("pv" "_DISABLE_C"
    COMMAND --client $<TARGET_FILE:paraview>
            --enable-bt
            -dr
            --test-directory=${PARAVIEW_TEST_DIR}
            --state=${CMAKE_CURRENT_BINARY_DIR}/exodusStateFile.3.14.1.pvsm
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LoadExodusStateFile.xml)

  #----------------------------------------------------------------------
  # Add test to test stereo rendering modes.
  add_client_tests("pv"
    --stereo
    --stereo-type=Interlaced
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/StereoInterlaced.xml)
  add_client_server_tests("pvcs"
    --stereo
    --stereo-type=Interlaced
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/StereoInterlaced.xml)
  add_client_render_server_tests("pvcrs"
    --stereo
    --stereo-type=Interlaced
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/StereoInterlaced.xml)
  add_client_tests("pv"
    --stereo
    --stereo-type=SplitViewportHorizontal
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/StereoSplitViewportHorizontal.xml)
  add_client_server_tests("pvcs"
    --stereo
    --stereo-type=SplitViewportHorizontal
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/StereoSplitViewportHorizontal.xml)
  add_client_render_server_tests("pvcrs"
    --stereo
    --stereo-type=SplitViewportHorizontal
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/StereoSplitViewportHorizontal.xml)
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
  ExportSceneSpreadSheetView
)

if (PARAVIEW_ENABLE_PYTHON)
  foreach(test_name ${paraview_python_verify_tests})
    add_test(NAME ${test_name}
      COMMAND ${CMAKE_COMMAND}
      -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:paraview>
      -DPVPYTHON_EXECUTABLE:FILEPATH=$<TARGET_FILE:pvpython>
      -DPARAVIEW_TEST_OUTPUT_DIR:PATH=${PARAVIEW_TEST_OUTPUT_DIR}
      -DTEST_NAME:STRING=${test_name}
      -DTEST_SCRIPT:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/${test_name}.xml
      -DTEST_DRIVER:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/${test_name}Verify.py
      -DTEMPORARY_DIR:PATH=${PARAVIEW_TEST_OUTPUT_DIR}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/PythonScriptTestDriver.cmake
    )
    set_tests_properties(${python_state_test} PROPERTIES LABELS "PARAVIEW")
  endforeach()
endif()
