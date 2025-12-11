include("${CMAKE_CURRENT_LIST_DIR}/gitlab_ci.cmake")

# Read the files from the build directory.
ctest_read_custom_files("${CTEST_BINARY_DIRECTORY}")

# Pick up from where the configure left off.
ctest_start(APPEND)

include(ProcessorCount)
ProcessorCount(nproc)
if (NOT "$ENV{CTEST_MAX_PARALLELISM}" STREQUAL "")
  if (nproc GREATER "$ENV{CTEST_MAX_PARALLELISM}")
    set(nproc "$ENV{CTEST_MAX_PARALLELISM}")
  endif ()
endif ()

# Default to a reasonable test timeout.
set(CTEST_TEST_TIMEOUT 100)

set(test_exclusions)

list(APPEND test_exclusions
  # see paraview/paraview#21440
  "\\.TraceExodus$"
  # see https://gitlab.kitware.com/paraview/paraview/-/issues/22349
  "\\.SplitViewTrace$"
  # see https://gitlab.kitware.com/paraview/paraview/-/issues/22478
  "\\.BivariateTextureRepresentation$"
  # see https://gitlab.kitware.com/paraview/paraview/-/issues/22694
  "\\.HyperTreeGridObliquePlaneCutter$"
  # Random segfault that would require deep investigation
  # https://gitlab.kitware.com/paraview/paraview/-/issues/21484
  "\\.ColorOpacityTableEditing$"
  # https://gitlab.kitware.com/paraview/paraview/-/issues/21656
  "\\.ShaderReplacements$"
  # https://gitlab.kitware.com/paraview/paraview/-/issues/21752
  "\\.ComparativeViewOverlay$"
  # https://gitlab.kitware.com/paraview/paraview/-/issues/17941
  "^paraviewPython-TestGeometryBoundsClobber$"
  )

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "_mpi")
  # tests that have issues in parallel
  list(APPEND test_exclusions
    # see paraview/paraview#20740
    "pvcs\\.ExplicitStructuredGridReal$"
    "pvcrs\\.ExplicitStructuredGridReal$"

    # see paraview/paraview#20741
    "pvcs\\.CategoricalAutomaticAnnotations$"
    "pvcrs\\.CategoricalAutomaticAnnotations$"

    # see https://gitlab.kitware.com/paraview/paraview/-/issues/23071
    "pvcs\\.LagrangianParticleTrackerParallelDistributed$"
    "pvcrs\\.LagrangianParticleTrackerParallelDistributed$"
    "pvcs\\.LagrangianParticleTrackerParallel$"
    "pvcrs\\.LagrangianParticleTrackerParallel$"
    "ParaViewExample-Plugins/LagrangianIntegrationModel$"
  )
endif()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "fedora")
  list(APPEND test_exclusions
    # These tests all seem to have some problem with the rendering order of
    # some components of the scenes that are being tested. Needs investigation.
    # https://gitlab.kitware.com/vtk/vtk/-/issues/18098
    "\\.BlockLinkedSelection$"
    "\\.BoxWidget$"
    "\\.CTHAMRClip$"
    "\\.MultiSliceWavelet$"
    "\\.NonConvexPolygon$"
    "\\.SelectionLinkMultiple$"
    "\\.SelectionModifiersBlocks$"
    "\\.SpreadSheet1$"
    "\\.VolumeCrop$"

    # https://gitlab.kitware.com/paraview/paraview/-/issues/22352
    "\\.FeatureEdgesFilterHTG$"
    "\\.FeatureEdgesRepresentationHTG$"

    # Transfer function image corruption
    # https://gitlab.kitware.com/paraview/paraview/-/issues/21428
    "\\.TransferFunction2DYScalars$"

    # "Structure does not match. You must use CopyStructure before calling this
    # method."
    # https://gitlab.kitware.com/paraview/paraview/-/issues/20692
    "^pvcrs\\.VisItBridgeLAMMPSDump$"
    "^pvcrs\\.VisItBridgeLAMMPSDump2$"
    "^pvcs\\.VisItBridgeLAMMPSDump$"
    "^pvcs\\.VisItBridgeLAMMPSDump2$"

    # Some X sync issue causing the images to be capture with
    # incorrect size, ignore for now.
    "\\.MultiSliceMultiBlock$"

    # https://gitlab.kitware.com/paraview/paraview/-/issues/21752
    "\\.ComparativeViewOverlay$"

    # https://gitlab.kitware.com/paraview/paraview/-/issues/22427
    "pqCoreKeySequences$"
    "pqWidgetspqTextEditTest$"
    "pv\\.ComputeArrayMagnitudeSetting$"
    "TestPythonView$"
    "pv\\.TooltipCopy$"

    # https://gitlab.kitware.com/paraview/paraview/-/issues/22801
    # There are warnings about initialization order confusion, but there are
    # then GL context errors. Not sure if these are related.
    "^paraviewPython-TestCatalystClient$"

    # https://gitlab.kitware.com/paraview/paraview/-/issues/22598
    "^pv\\.HelpWindowHistory$"

    # https://gitlab.kitware.com/paraview/paraview/-/issues/22600
    "\\.DigitalRockPhysicsAnalysisFilter$"
    "\\.DigitalRockPhysicsExplodeFilter$"

    # https://gitlab.kitware.com/paraview/paraview/-/issues/22601
    "\\.ConvertToMolecule$"

    # Timeouts https://gitlab.kitware.com/paraview/paraview/-/issues/20108
    "^ParaViewExample-Catalyst$"

    # https://gitlab.kitware.com/paraview/paraview/-/issues/21462
    "\\.UndoRedo1"

    # https://gitlab.kitware.com/paraview/paraview/-/issues/22743
    "^pv.SimpleSSHServerTermExec$"

    # https://gitlab.kitware.com/paraview/paraview/-/issues/23073
    "^pvcrs\\.AMReXParticlesReader$"
    "^pvcrs\\.ComputeConnectedSurfaceProperties$"
    "^pvcrs\\.CONVERGECFDReaderWithVisItBridge$"
    "^pvcrs\\.EDLWithSubsampling$"
    "^pvcrs\\.ExtractLevel$"
    "^pvcrs\\.FidesReaderADIOS2$"
    "^pvcrs\\.Glyph3DRepresentation$"
    "^pvcrs\\.LoadState$"
    "^pvcrs\\.MemoryInspectorPanel$"
    "^pvcrs\\.MultiBlockInspectorMultiBlock$"
    "^pvcrs\\.OctreeImageFilters$"
    "^pvcrs\\.OMFReader$"
    "^pvcrs\\.PropertyLink$"
    "^pvcrs\\.ReadPartitionedCGNS$"
    "^pvcrs\\.SelectedProxyPanelVisibility$"
    "^pvcrs\\.TestGroupDataFromTimeSeries$"
    "^pvcrs\\.TestOpacityRendering$"
    "^pvcrs\\.UnstructuredVolumeRenderingVectorComponent$"

    # https://gitlab.kitware.com/paraview/paraview/-/issues/23165
    "^pv\\.FileDialogAllFavorites$"
    "^pvcrs\\.FileDialogAllFavorites$"
  )

  if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "static")
    list(APPEND test_exclusions
      # https://gitlab.kitware.com/paraview/paraview/-/issues/22398
      "^ParaViewExample-Catalyst2/PythonFullExample$"
      "^ParaViewExample-Catalyst2/PythonSteeringExample$")
  endif ()

  if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "viskoresoverride")
    list(APPEND test_exclusions
      # https://gitlab.kitware.com/paraview/paraview/-/issues/22941
      "^pv\\.AdaptiveResampleToImage$"
      "^pvcs\\.AdaptiveResampleToImage$")
  endif ()

  if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "_qt")
    list(APPEND test_exclusions
      # Test fails when PARAVIEW_ENABLE_VISITBRIDGE is ON (it doesn't know which Fluent reader to chose)
      "FluentReaderZoneSelection$")
  endif ()

endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos")
  list(APPEND test_exclusions
    # Known-bad
    "\\.PreviewFontScaling$"

    # Unstructured grid volume rendering (paraview/paraview#19130)
    "\\.MultiBlockVolumeRendering$"
    "\\.UnstructuredVolumeRenderingVectorComponent$"

    # https://gitlab.kitware.com/paraview/paraview/-/issues/21421
    "\\.PythonEditorRun$"

    # https://gitlab.kitware.com/paraview/paraview/-/issues/22674
    "^paraviewPython-Batch-TestStereoSaveScreenshot$"
    "^paraviewPython-TestStereoSaveScreenshot$"

    # https://gitlab.kitware.com/paraview/paraview/-/issues/22800
    "^paraviewPython-SaveTransparentImages$"

    # https://gitlab.kitware.com/paraview/paraview/-/issues/22676
    "^paraviewPython-TestHTGContourMonoHT$"
    "^paraviewPython-TestHTG3DContourPolyhedron$"

    # https://gitlab.kitware.com/paraview/paraview/-/issues/22696
    "^pv\\.LagrangianSurfaceHelperComposite$"

    # https://gitlab.kitware.com/paraview/paraview/-/issues/22827
    # Unclassified
    "^pv\\.BivariateNoiseRepresentation$"
    "^pv\\.DecimatePolyline$"
    "^pvcrs\\.DecimatePolyline$"
    "^pvcs\\.DecimatePolyline$"
    "^pv\\.TestExtrusionRepresentationCellData$"
    "^pvcrs\\.GroupDataSetOutputType$"
    "^pvcs\\.GroupDataSetOutputType$"
    "^pv\\.UndoRedo1$"
    "^pvcrs\\.UndoRedo1$"
    "^pvcs\\.UndoRedo1$"

    # https://gitlab.kitware.com/paraview/paraview/-/issues/22823
    # Floating point subtleties
    "^pv\\.AnimateProperty$"

    # https://gitlab.kitware.com/paraview/paraview/-/issues/22824
    # Shading differences
    "^pv\\.PointGaussianMultiBlockDataSet$"
    "^pvcrs\\.PointGaussianMultiBlockDataSet$"
    "^pvcs\\.PointGaussianMultiBlockDataSet$"
    "^pv\\.PointGaussianNoScaleTransferFunction$"
    "^pvcrs\\.PointGaussianNoScaleTransferFunction$"
    "^pvcs\\.PointGaussianNoScaleTransferFunction$"
    "^pv\\.UniformInverseTransformSamplingGlyph$"
    "^pvcrs\\.UniformInverseTransformSamplingGlyph$"
    "^pvcs\\.UniformInverseTransformSamplingGlyph$"
    "^pv\\.CONVERGECFDReader$"
    "^pvcrs\\.CONVERGECFDReader$"
    "^pvcs\\.CONVERGECFDReader$"
    # The following tests seem to have some geometry differences too
    "^pv\\.AxisAlignedCutterMBHierarchy$"
    "^pvcrs\\.AxisAlignedCutterMBHierarchy$"
    "^pvcs\\.AxisAlignedCutterMBHierarchy$"
    "^pv\\.AxisAlignedCutterPDCNoHierarchy$"
    "^pvcrs\\.AxisAlignedCutterPDCNoHierarchy$"
    "^pvcs\\.AxisAlignedCutterPDCNoHierarchy$"
    "^pv\\.AxisAlignedPDCNoHierarchy$"
    "^pvcrs\\.AxisAlignedPDCNoHierarchy$"
    "^pvcs\\.AxisAlignedPDCNoHierarchy$"

    # https://gitlab.kitware.com/paraview/paraview/-/issues/22825
    # M4 geometry filter
    "^pvcs-tile-display\\.LinkCameraFromView-1x1$"
    "^pv\\.FeatureEdgesFilterHTG$"
    "^pvcrs\\.FeatureEdgesFilterHTG$"
    "^pvcs\\.FeatureEdgesFilterHTG$"
    "^pv\\.FeatureEdgesRepresentationHTG$"
    "^pvcrs\\.FeatureEdgesRepresentationHTG$"
    "^pvcs\\.FeatureEdgesRepresentationHTG$"
    "^pv\\.MultipleColorOnSelection$"
    "^pvcs\\.MultipleColorOnSelection$"
    )
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_arm64")
  list(APPEND test_exclusions
    # https://gitlab.kitware.com/paraview/paraview/-/issues/20743
    "^pv\\.TestExtrusionRepresentationCellData$"
    # https://gitlab.kitware.com/paraview/paraview/-/issues/22353
    "\\.FeatureEdgesFilterHTG$"
    "\\.FeatureEdgesRepresentationHTG$"
    # https://gitlab.kitware.com/paraview/paraview/-/issues/21462
    "\\.UndoRedo1"
    # https://gitlab.kitware.com/paraview/paraview/-/issues/21768
    "^pv\\.ServerConnectConfigured$"
    # https://gitlab.kitware.com/paraview/paraview/-/issues/21786
    "^pv\\.MultipleColorOnSelection"
    "^pvcs\\.MultipleColorOnSelection"
    # https://gitlab.kitware.com/paraview/paraview/-/merge_requests/7083
    "^pvcs\\.GroupDataSetOutputType$"
    "^pvcrs\\.GroupDataSetOutputType$"
    )
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  list(APPEND test_exclusions
    # See https://gitlab.kitware.com/paraview/paraview/-/issues/20282
    "\\.ColorByCellDataStringArray$"
    "\\.IndexedLookupInitialization$"
    "\\.LoadStateMultiView$"
    "\\.PreviewFontScaling$"
    "\\.SelectPointsTrace$"
    "\\.UnstructuredVolumeRenderingVectorComponent$"
    "^pv\\.CompositeSurfaceSelection$"
    "^pv\\.ExportFilteredColumnsSpreadsheet$"
    "^pv\\.ExportSpreadsheetFormatting$"
    "^pv\\.SaveStateAndScreenshot$"
    "^pvcrs\\.FindDataDialog$"
    "^pvcs\\.ColorOpacityTableEditorHistogram$"
    "^pvcs\\.SplitViewTrace$"

    # The generated paths are too long and don't work in MSVC.
    # See https://gitlab.kitware.com/paraview/paraview/-/issues/20589
    "^pv\\.TestDevelopmentInstall$"

    # This test has some weird rendering artifacts. It looks like the Intel
    # rendering bug, but our machines all use nVidia cards today.
    "^paraviewPython-TestColorHistogram$"

    # https://gitlab.kitware.com/paraview/paraview/-/issues/22801
    # There are warnings about initialization order confusion, but there are
    # then GL context errors. Not sure if these are related.
    "^paraviewPython-TestCatalystClient$"

    # Fails on windows.
    "pqWidgetsHeaderViewCheckState"

    # Fails sporadically (paraview/paraview#20702)
    "^pv\\.TestPythonConsole$"

    # Flaky with timeouts
    "^pvcs\\.UndoRedo1"

    # Fails on windows-vs2019-qt
    # See https://gitlab.kitware.com/paraview/paraview/-/issues/21771
    "^pv\\.HelpWindowHistory$"

    # Flaky for some reasons
    # https://gitlab.kitware.com/paraview/paraview/-/issues/21421
    "\\.PythonEditorRun$"

    # Fails on windows-vs2022-qt
    # https://gitlab.kitware.com/paraview/paraview/-/merge_requests/7083
    "^pvcs\\.GroupDataSetOutputType$"
    "^pvcrs\\.GroupDataSetOutputType$"
    )
endif ()

if ("$ENV{CC}" STREQUAL "icx")
  list(APPEND test_exclusions
    # OpenMPI outputs text that ends up getting detected as a test failure.
    "^ParaViewExample-Catalyst2/CxxImageDataExample$"

    # Known-bad https://gitlab.kitware.com/paraview/paraview/-/issues/20108
    "^ParaViewExample-Catalyst$"

    # Shared memory limitations.
    "^Catalyst::WaveletMiniApp.package_test$"
    "^Catalyst::WaveletMiniApp.package_test_zip$"
    "^ParaView::RemotingServerManagerPython-TestGlobbing$"
    "^SurfaceLIC-OfficeContour-Batch$"
    "^SurfaceLIC-OfficeSlices-Batch$"
    )
endif ()

string(REPLACE ";" "|" test_exclusions "${test_exclusions}")
if (test_exclusions)
  set(test_exclusions "(${test_exclusions})")
endif ()

ctest_test(APPEND
  PARALLEL_LEVEL "${nproc}"
  TEST_LOAD "${nproc}"
  RETURN_VALUE test_result
  EXCLUDE "${test_exclusions}"
  OUTPUT_JUNIT "${CTEST_BINARY_DIRECTORY}/junit.xml"
  REPEAT UNTIL_PASS:3)
ctest_submit(PARTS Test)

include("${CMAKE_CURRENT_LIST_DIR}/ctest_annotation.cmake")
if (DEFINED build_id)
  ctest_annotation_report("${CTEST_BINARY_DIRECTORY}/annotations.json"
    "Build Summary" "https://open.cdash.org/build/${build_id}"
    "All Tests"     "https://open.cdash.org/viewTest.php?buildid=${build_id}"
    "Test Failures" "https://open.cdash.org/viewTest.php?onlyfailed&buildid=${build_id}"
    "Tests Not Run" "https://open.cdash.org/viewTest.php?onlynotrun&buildid=${build_id}"
    "Test Passes"   "https://open.cdash.org/viewTest.php?onlypassed&buildid=${build_id}"
  )
endif ()

if (test_result)
  message(FATAL_ERROR
    "Failed to test")
endif ()
