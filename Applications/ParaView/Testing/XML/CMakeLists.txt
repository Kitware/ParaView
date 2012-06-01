SET (TESTS_WITHOUT_BASELINES
  ${CMAKE_CURRENT_SOURCE_DIR}/AboutDialog.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/OpenHelp.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ServerConnectDialog.xml
)

SET (TESTS_WITH_INLINE_COMPARES
  ${CMAKE_CURRENT_SOURCE_DIR}/TestIsoVolume.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/LoadStateMultiView.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/RenderNan.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SaveLargeScreenshot.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/SLACTools.xml
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
    ${CMAKE_CURRENT_SOURCE_DIR}/VisItBridgeNek.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/VisItBridgePdb.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/VisItBridgePixie.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/VisItBridgeSamrai.xml)
ENDIF(NOT PARAVIEW_USE_VISITBRIDGE)


SET (TESTS_WITH_BASELINES
  ${TESTS_WITH_BASELINES}
  ${CMAKE_CURRENT_SOURCE_DIR}/CTHAMRBaseline.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CTHAMRContour.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CTHAMRDualClip.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CTHAMRClip.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CTHAMRMaterialInterfaceFilter.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CTHDerivedDensity2DClyinder.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/AnimatePipelineTime.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/3DWidgetInCustomFilter.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CalcParens.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Calculator.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CameraLink.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Clip.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/NewColorEditor1.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ColorEditor2.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/NewColorEditor3.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ColorEditor4.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ComparativeVisPanel.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CompositeSurfaceSelection.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Contour.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ContourRange.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CreateDelete.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CustomFilter.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/CustomSourceProbe.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/D3SmallCells.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/DualSphereAnimation.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/EnSight.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExodusModeShapes.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExodusXML.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExportX3dPOVVRML.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExTimeseries.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExTimeseries2.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExtractBlock.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ExtractLevel.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/EyeDomeLighting.xml
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
  ${CMAKE_CURRENT_SOURCE_DIR}/LoadStateMultiView.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/NormalGlyphs.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/NiftiReaderWriterPlugin.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/OpenSaveData.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Plot3DReader.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PlotEdges.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PlotEdges2.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ProbePicking.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PropertyConversion.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PropertyConversion1.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PropertyConversion2.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/PropertyConversion3.xml
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
  ${CMAKE_CURRENT_SOURCE_DIR}/TabbedViews.xml
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

# Probe picking does not work in render server mode
set(ProbePicking_DISABLE_CRS TRUE)

# only enable find data tests if numpy is available
include("TestNumPy")

IF ("1" STREQUAL ${HAS_NUMPY})
  SET(TESTS_WITH_BASELINES ${TESTS_WITH_BASELINES} ${CMAKE_CURRENT_SOURCE_DIR}/FindDataDialog.xml)
ENDIF ("1" STREQUAL ${HAS_NUMPY})

# These tests run using the new properties panel
set(AboutDialog_USE_NEW_PANELS TRUE)
set(AnimatePipelineTime_USE_NEW_PANELS TRUE)
set(CalcParens_USE_NEW_PANELS TRUE)
set(Calculator_USE_NEW_PANELS TRUE)
set(CameraLink_USE_NEW_PANELS TRUE)
set(Clip_USE_NEW_PANELS TRUE)
set(ColorEditor2_USE_NEW_PANELS TRUE)
set(ColorEditor4_USE_NEW_PANELS TRUE)
set(CompositeSurfaceSelection_USE_NEW_PANELS TRUE)
set(ContourRange_USE_NEW_PANELS TRUE)
set(Contour_USE_NEW_PANELS TRUE)
set(CreateDelete_USE_NEW_PANELS TRUE)
set(CTHAMRBaseline_USE_NEW_PANELS TRUE)
set(CTHDerivedDensity2DClyinder_USE_NEW_PANELS TRUE)
set(CustomFilter_USE_NEW_PANELS TRUE)
set(CustomSourceProbe_USE_NEW_PANELS TRUE)
set(D3SmallCells_USE_NEW_PANELS TRUE)
set(DualSphereAnimation_USE_NEW_PANELS TRUE)
set(EnSight_USE_NEW_PANELS TRUE)
set(ExodusXML_USE_NEW_PANELS TRUE)
set(ExportX3dPOVVRML_USE_NEW_PANELS TRUE)
set(ExTimeseries2_USE_NEW_PANELS TRUE)
set(ExTimeseries_USE_NEW_PANELS TRUE)
set(ExtractBlock_USE_NEW_PANELS TRUE)
set(ExtractLevel_USE_NEW_PANELS TRUE)
set(EyeDomeLighting_USE_NEW_PANELS TRUE)
set(FFTOverTime_USE_NEW_PANELS TRUE)
set(FindDataDialog_USE_NEW_PANELS TRUE)
set(Flow2_USE_NEW_PANELS TRUE)
set(Flow_USE_NEW_PANELS TRUE)
set(Fractal2D_USE_NEW_PANELS TRUE)
set(GridConnectivity_USE_NEW_PANELS TRUE)
set(H5PartReaderPlugin_USE_NEW_PANELS TRUE)
set(LoadPlugins_USE_NEW_PANELS TRUE)
set(LoadSaveStateAnimation_USE_NEW_PANELS TRUE)
set(LoadState_USE_NEW_PANELS TRUE)
set(NiftiReaderWriterPlugin_USE_NEW_PANELS TRUE)
set(NewColorEditor1_USE_NEW_PANELS TRUE)
set(NewColorEditor3_USE_NEW_PANELS TRUE)
set(NormalGlyphs_USE_NEW_PANELS TRUE)
set(OpenSaveData_USE_NEW_PANELS TRUE)
set(ProbePicking_USE_NEW_PANELS TRUE)
set(Plot3DReader_USE_NEW_PANELS TRUE)
set(PlotEdges2_USE_NEW_PANELS TRUE)
set(PlotEdges_USE_NEW_PANELS TRUE)
set(PropertyConversion1_USE_NEW_PANELS TRUE)
set(PropertyConversion2_USE_NEW_PANELS TRUE)
set(PropertyConversion3_USE_NEW_PANELS TRUE)
set(PropertyConversion_USE_NEW_PANELS TRUE)
set(PropertyLink_USE_NEW_PANELS TRUE)
set(RectilinearFractal_USE_NEW_PANELS TRUE)
set(RenderNan_USE_NEW_PANELS TRUE)
set(RepresentationSelector_USE_NEW_PANELS TRUE)
set(SaveColorMap_USE_NEW_PANELS TRUE)
set(SaveCSV_USE_NEW_PANELS TRUE)
set(SelectReader_USE_NEW_PANELS TRUE)
set(SimpleInteraction_USE_NEW_PANELS TRUE)
set(Slice_USE_NEW_PANELS TRUE)
set(SpreadSheet1_USE_NEW_PANELS TRUE)
set(SpreadSheet2_USE_NEW_PANELS TRUE)
set(SPTimeseries_USE_NEW_PANELS TRUE)
set(SpyPlotHistoryReader_USE_NEW_PANELS TRUE)
set(StreamTracerUpdates_USE_NEW_PANELS TRUE)
set(TabbedViews_USE_NEW_PANELS TRUE)
set(TemporalInterpolator_USE_NEW_PANELS TRUE)
set(TemporalShiftScale_USE_NEW_PANELS TRUE)
set(Tessellate_USE_NEW_PANELS TRUE)
set(TestIsoVolume_USE_NEW_PANELS TRUE)
set(UndoRedo1_USE_NEW_PANELS TRUE)
set(UndoRedo5_USE_NEW_PANELS TRUE)
set(UndoRedo6_USE_NEW_PANELS TRUE)
set(UndoRedo7_USE_NEW_PANELS TRUE)
set(UndoRedo_USE_NEW_PANELS TRUE)
set(UnstructuredOutline_USE_NEW_PANELS TRUE)
set(VariableSelector1_USE_NEW_PANELS TRUE)
set(VariableSelector_USE_NEW_PANELS TRUE)
set(ViewSettingsDialog_USE_NEW_PANELS TRUE)
set(XdmfReadImageDataCollection_USE_NEW_PANELS TRUE)
set(XdmfReadImageData_USE_NEW_PANELS TRUE)
set(XdmfReadRectilinearGridCollection_USE_NEW_PANELS TRUE)
set(XdmfReadRectilinearGrid_USE_NEW_PANELS TRUE)
set(XdmfRead_USE_NEW_PANELS TRUE)
set(XYChart_USE_NEW_PANELS TRUE)
set(XYHistogram_USE_NEW_PANELS TRUE)
set(ZLibXDMF_USE_NEW_PANELS TRUE)

# These tests should not be run along with those using the new panels
set(3DWidgetInCustomFilter_BREAK TRUE)
set(Threshold_BREAK TRUE)
set(ExodusModeShapes_BREAK TRUE)

# These tests cannot be run using ctest -j since they are affected by focus
# changed events.
set (AnimatePipelineTime_FORCE_SERIAL TRUE)

# Set properties for CTH tests
set(CTHAMRBaseline_BREAK TRUE)

set(CTHAMRContour_BREAK TRUE)
set(CTHAMRContour_DISABLE_CS TRUE)
set(CTHAMRContour_DISABLE_CRS TRUE)

set(CTHAMRDualClip_BREAK TRUE)
set(CTHAMRDualClip_DISABLE_CS TRUE)
set(CTHAMRDualClip_DISABLE_CRS TRUE)

set(CTHAMRClip_BREAK TRUE)
set(CTHAMRClip_DISABLE_CS TRUE)
set(CTHAMRClip_DISABLE_CRS TRUE)

set(CTHAMRMaterialInterfaceFilter_BREAK TRUE)
set(CTHAMRMaterialInterfaceFilter_DISABLE_CS TRUE)
set(CTHAMRMaterialInterfaceFilter_DISABLE_CRS TRUE)

set (TestIsoVolume_BREAK TRUE)
set (TabbedViews_BREAK TRUE)

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

set (NewColorEditor1_BREAK TRUE)
set (ColorEditor2_BREAK TRUE)
set (NewColorEditor3_BREAK TRUE)
set (ColorEditor4_BREAK TRUE)
set (GridConnectivity_BREAK TRUE)
set (H5PartReaderPlugin_BREAK TRUE)
SET (PlotEdges2_BREAK TRUE)
SET (PropertyConversion2_BREAK TRUE)
SET (PropertyConversion3_BREAK TRUE)
set (ReadXMLPolyDataFileSeries_BREAK TRUE)
set (ExodusXML_BREAK TRUE)
set (EyeDomeLighting_BREAK TRUE)
SET (ComparativeVisPanel_BREAK TRUE)
SET (SpreadSheet1_BREAK TRUE)
SET (SPTimeseries_BREAK TRUE)
SET (XYChart_BREAK TRUE)
SET (XYHistogram_BREAK TRUE)

SET(ExtractLevel_BREAK TRUE)

SET(Fractal2D_BREAK TRUE)

# Add image threshold overrides for tests.
# Generally all tests with wireframes need higher thresholds.
SET (Flow2_THRESHOLD 15)
SET (UndoRedo1_THRESHOLD 15)
SET (UndoRedo4_THRESHOLD 15)

# Since often there are issues with fonts etc.
SET (XYChart_THRESHOLD 150)

SET (XYHistogram_THRESHOLD 60)

# The color and opacity are often different on different machines.
SET (NewColorEditor1_THRESHOLD 60)
SET (NewColorEditor3_THRESHOLD 60)

SET (SLACTools_DISABLE_CS TRUE)
SET (SLACTools_DISABLE_CRS TRUE)
SET (SLACTools_BREAK TRUE)
SET(SLACTools_THRESHOLD 20)

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

set (LoadStateMultiView_BREAK TRUE)
set (RenderNan_BREAK TRUE)

# NewColorEditor3 does volume rendering of structrued data which required remote
# rendering in client-serve mode.
SET (NewColorEditor3_DISABLE_CS TRUE)
SET (NewColorEditor3_DISABLE_CRS TRUE)
# NewColorEditor1 has a clip widget that can not be handled properly by
# the collaboration render window yet, so disable for now.
SET (NewColorEditor1_DISABLE_CRS TRUE)

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
  SET (SLACTools_DISABLE_C TRUE)
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

# Set image threshold overrides for the tests.
SET(SaveLargeScreenshot_THRESHOLD 10000)

IF(NOT PARAVIEW_BUILD_PLUGIN_EyeDomeLighting)
  SET(EyeDomeLighting_DISABLE_C TRUE)
ENDIF()

# Make these tests use reverse connection.
SET (CutMulti_REVERSE_CONNECT TRUE)

##########################################################
# List of known incompatible tests for Collaboration
#
#   Contains Undo/Redo actions:
#     - Clip, GlobalProperties, UndoRedo, UndoRedo[1-8]
#
#   Contains Plugin Loading actions:
#     - LoadPlugins, NiftiReaderWriterPlugin, EyeDomeLighting, H5PartReaderPlugin
#
#   Does not work in CS or CRS mode
#     - ViewSettingsDialog, NewColorEditor3, RectilinearFractal, NewColorEditor
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
set (AnimatePipelineTime_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (CalcParens_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (Calculator_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (CameraLink_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (ColorEditor2_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
#set (ColorEditor4_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
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
set (TemporalInterpolator_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
set (TemporalShiftScale_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
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
#set (XYChart_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})
set (ZLibXDMF_ENABLE_COLLAB ${PARAVIEW_COLLABORATION_TESTING})


set(TESTS_WITH_MULTI_SERVERS_3
  ${CMAKE_CURRENT_SOURCE_DIR}/TestMultiServer3.xml
)

IF (PARAVIEW_DATA_ROOT)
  set (TESTS_WITHOUT_BASELINES
    ${TESTS_WITHOUT_BASELINES}
    ${TESTS_WITH_INLINE_COMPARES})
ENDIF (PARAVIEW_DATA_ROOT)

add_client_tests("pv"
  TEST_SCRIPTS ${TESTS_WITHOUT_BASELINES}
  )

IF (PARAVIEW_DATA_ROOT)
  add_client_tests("pv"
    BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
    TEST_SCRIPTS ${TESTS_WITH_BASELINES}
  )

  add_client_server_tests("pvcs"
    BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
    TEST_SCRIPTS ${TESTS_WITH_BASELINES}
  )

  add_client_server_tests("pvcs"
    TEST_SCRIPTS ${TESTS_WITH_INLINE_COMPARES}
  )

  IF (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
    add_client_render_server_tests("pvcrs"
      BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
      TEST_SCRIPTS ${TESTS_WITH_BASELINES}
    )

    add_client_render_server_tests("pvcrs"
      TEST_SCRIPTS ${TESTS_WITH_INLINE_COMPARES}
    )
  ENDIF (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)

# TODO: remote rendering tests and reverse connect tests.

  add_multi_client_tests("pvcs-collab"
    BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
    TEST_SCRIPTS ${TESTS_WITH_BASELINES})

  add_multi_server_tests("pvcs-multi-servers" 3
     BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
     TEST_SCRIPTS ${TESTS_WITH_MULTI_SERVERS_3})

  add_tile_display_tests("pvcs-tile-display" 2 1
     BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
     TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/TileDisplayScatterPlot-2x1.xml)

  add_tile_display_tests("pvcs-tile-display" 2 2
     BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
     TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/TileDisplayScatterPlot-2x2.xml)

  add_tile_display_tests("pvcs-tile-display" 1 2
     BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
     TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/TileDisplay3DTesting-1x2.xml)

  add_tile_display_tests("pvcs-tile-display" 2 1
     BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
     TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/TileDisplay3DTesting-2x1.xml)

  add_tile_display_tests("pvcs-tile-display" 2 2
     BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
     TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/TileDisplay3DTesting-2x2.xml)

ENDIF (PARAVIEW_DATA_ROOT)
