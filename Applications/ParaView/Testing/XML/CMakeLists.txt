SET (TESTS_WITHOUT_BASELINES
  ${CMAKE_CURRENT_SOURCE_DIR}/AboutDialog.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/OpenHelp.xml
)

SET (TESTS_WITH_BASELINES)
IF(NOT PARAVIEW_USE_VISITBRIDGE)
  # Analyze reader test was conflicting with the reader from VisIt. So
  # disabling it.
  SET (TESTS_WITH_BASELINES ${TESTS_WITH_BASELINES}
    ${CMAKE_CURRENT_SOURCE_DIR}/AnalyzeReaderWriterPlugin.xml)
ELSE(NOT PARAVIEW_USE_VISITBRIDGE)
  SET (TESTS_WITH_BASELINES ${TESTS_WITH_BASELINES}
    ${CMAKE_CURRENT_SOURCE_DIR}/VisItBridgeChombo.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/VisItBridgeEnzo.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/VisItBridgeNas.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/VisItBridgePdb.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/VisItBridgePixie.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/VisItBridgeSamrai.xml)
ENDIF(NOT PARAVIEW_USE_VISITBRIDGE)


SET (TESTS_WITH_BASELINES
  ${TESTS_WITH_BASELINES}
  ${CMAKE_CURRENT_SOURCE_DIR}/AnimatePipelineTime.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/3DWidgetInCustomFilter.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CalcParens.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Calculator.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CameraLink.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Clip.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ColorEditor1.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ColorEditor2.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ColorEditor3.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ColorEditor4.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ComparativeVisPanel.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CompositeSurfaceSelection.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Contour.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ContourRange.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CustomFilter.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CustomSourceProbe.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/D3SmallCells.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/DualSphereAnimation.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/EnSight.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExodusIIPanel.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExodusModeShapes.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExodusXML.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExportX3dPOVVRML.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExTimeseries.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExTimeseries2.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExtractBlock.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExtractLevel.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/EyeDomeLighting.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/FindDataDialog.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/FFTOverTime.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Flow.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Flow2.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Fractal2D.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/GlobalProperties.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/GridConnectivity.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/H5PartReaderPlugin.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/LoadPlugins.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/LoadSaveStateAnimation.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/LoadState.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/NormalGlyphs.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/NiftiReaderWriterPlugin.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ObjectInspectorSphere.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/OpenSaveData.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Plot3DReader.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PlotEdges.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PlotEdges2.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PropertyConversion.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PropertyConversion1.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PropertyConversion2.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PropertyLink.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ReadXMLPolyDataFileSeries.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/RectilinearFractal.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/RepresentationSelector.xml
#  ${CMAKE_CURRENT_SOURCE_DIR}/SaveAnimationGeometry.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SaveColorMap.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SaveCSV.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SelectReader.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SimpleInteraction.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Slice.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SpreadSheet1.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SpreadSheet2.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SPTimeseries.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SpyPlotHistoryReader.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/StreamTracerUpdates.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/TemporalInterpolator.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/TemporalShiftScale.xml
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
  ${CMAKE_CURRENT_SOURCE_DIR}/VariableSelector.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/VariableSelector1.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ViewSettingsDialog.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/XdmfRead.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/XdmfReadImageData.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/XdmfReadImageDataCollection.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/XdmfReadRectilinearGrid.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/XdmfReadRectilinearGridCollection.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/XdmfReadStructuredGrid.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/XdmfReadStructuredGridCollection.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/XdmfGridAttributes.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/XYChart.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/XYHistogram.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ZLibXDMF.xml
  #  ${CMAKE_CURRENT_SOURCE_DIR}/XYBarChart.xml
)

# Mark tests that change change state considerably requires a restart of the
# test suite when grouping tests together for faster playback.
SET (GlobalProperties_BREAK TRUE)
  # <-- since this changes global colors.
SET (XdmfReadImageData_BREAK TRUE)
  # <-- since this changes light kit.
SET (UndoRedo3_BREAK TRUE)
SET (UndoRedo6_BREAK TRUE)
  # <-- since this changes view settings.
SET (UndoRedo1_BREAK TRUE)
SET (UndoRedo2_BREAK TRUE)
SET (UndoRedo7_BREAK TRUE)
  # <-- since these are just long tests, they result in timeouts.

SET (ViewSettingsDialog_BREAK TRUE)
SET (UnstructuredOutline_BREAK TRUE)
  # <-- since this changes view settings.

set (ColorEditor1_BREAK TRUE)
set (ColorEditor2_BREAK TRUE)
set (ColorEditor3_BREAK TRUE)
set (ColorEditor4_BREAK TRUE)
set (GridConnectivity_BREAK TRUE)
set (H5PartReaderPlugin_BREAK TRUE)
SET (PlotEdges2_BREAK TRUE)
SET (PropertyConversion2_BREAK TRUE)
set (ReadXMLPolyDataFileSeries_BREAK TRUE)
set (ExodusXML_BREAK TRUE)
set (EyeDomeLighting_BREAK TRUE)
SET (ComparativeVisPanel_BREAK TRUE)
SET (SpreadSheet1_BREAK TRUE)
SET (SPTimeseries_BREAK TRUE)
SET (XYChart_BREAK TRUE)
SET (XYHistogram_BREAK TRUE)

SET(ExtractLevel_BREAK TRUE)


# Add image threshold overrides for tests.
# Generally all tests with wireframes need higher thresholds.
SET (Flow2_THRESHOLD 15)
SET (UndoRedo1_THRESHOLD 15)
SET (UndoRedo4_THRESHOLD 15)

# Since often there are issues with fonts etc.
SET (XYChart_THRESHOLD 150)

SET (XYHistogram_THRESHOLD 60)

# Histogram filter produces different results when running in parallel, so
# disable it.
IF (PARAVIEW_USE_MPI)
  SET (XYHistogram_DISABLE_CS TRUE)
  SET (XYHistogram_DISABLE_CRS TRUE)

  SET (SpreadSheet1_DISABLE_CS TRUE)
  SET (SpreadSheet1_DISABLE_CRS TRUE)

  # The hierchical fractal source is a temporary testing source and it does not
  # create the dataset correctly in parallel. Since it's a testing source, I am
  # just going to disable the test in parallel. We can fix the source when
  # needed.
  SET (RectilinearFractal_DISABLE_CS TRUE)
  SET (RectilinearFractal_DISABLE_CRS TRUE)
ENDIF (PARAVIEW_USE_MPI)

# Composite Surface Selection is currently broken in everything but bultin
SET (CompositeSurfaceSelection_DISABLE_CS TRUE)
SET (CompositeSurfaceSelection_DISABLE_CRS TRUE)

# Clip test has object picking which is not supported in client-render-server
# mode.
SET (Clip_DISABLE_CRS TRUE)


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
SET (FFTOverTime_DISABLE_CRS TRUE)

# ColorEditor3 does volume rendering of structrued data which required remote
# rendering in client-serve mode.
SET (ColorEditor3_DISABLE_CS TRUE)
SET (ColorEditor3_DISABLE_CRS TRUE)

# ViewSettingsDialog uses texture background which is currently supported only
# in local render mode.
SET (ViewSettingsDialog_DISABLE_CS TRUE)
SET (ViewSettingsDialog_DISABLE_CRS TRUE)

# Plugins are only built as shared libraries.
IF (NOT BUILD_SHARED_LIBS)
  SET (AnalyzeReaderWriterPlugin_DISABLE_C TRUE)
  SET (H5PartReaderPlugin_DISABLE_C TRUE)
  SET (LoadPlugins_DISABLE_C TRUE)
  SET (NiftiReaderWriterPlugin_DISABLE_C TRUE)
ENDIF (NOT BUILD_SHARED_LIBS)
# There should be a client server specific version of this test.
SET (AnalyzeReaderWriterPlugin_DISABLE_CS TRUE)
SET (EyeDomeLighting_DISABLE_CS TRUE)
SET (H5PartReaderPlugin_DISABLE_CS TRUE)
SET (LoadPlugins_DISABLE_CS TRUE)
SET (NiftiReaderWriterPlugin_DISABLE_CS TRUE)
SET (AnalyzeReaderWriterPlugin_DISABLE_CRS TRUE)
SET (EyeDomeLighting_DISABLE_CRS TRUE)
SET (H5PartReaderPlugin_DISABLE_CRS TRUE)
SET (LoadPlugins_DISABLE_CRS TRUE)
SET (NiftiReaderWriterPlugin_DISABLE_CRS TRUE)

IF(NOT PARAVIEW_BUILD_PLUGIN_EyeDomeLighting)
  SET(EyeDomeLighting_DISABLE_C TRUE)
ENDIF()

# Make these tests use reverse connection.
SET (CutMulti_REVERSE_CONNECT TRUE)

add_client_tests("pv"
  TEST_SCRIPTS ${TESTS_WITHOUT_BASELINES}
  PARALLEL
  )

IF (PARAVIEW_DATA_ROOT)
  add_client_tests("pv"
    BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
    TEST_SCRIPTS ${TESTS_WITH_BASELINES}
    PARALLEL
  )

  add_client_server_tests("pvcs"
    BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
    TEST_SCRIPTS ${TESTS_WITH_BASELINES}
  )

  IF (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
    add_client_render_server_tests("pvcrs"
      BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
      TEST_SCRIPTS ${TESTS_WITH_BASELINES}
    )
  ENDIF (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)

# TODO: remote rendering tests and reverse connect tests.

ENDIF (PARAVIEW_DATA_ROOT)
