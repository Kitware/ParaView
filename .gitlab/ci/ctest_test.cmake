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

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "_mpi")
  # tests that have issues in parallel
  list(APPEND test_exclusions
    # see paraview/paraview#20740
    "pvcs\\.ExplicitStructuredGridReal$"
    "pvcrs\\.ExplicitStructuredGridReal$"

    # see paraview/paraview#20741
    "pvcs\\.CategoricalAutomaticAnnotations$"
    "pvcrs\\.CategoricalAutomaticAnnotations$"
  )
endif()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "fedora")
  list(APPEND test_exclusions
    # These tests all seem to have some problem with the rendering order of
    # some components of the scenes that are being tested. Needs investigation.
    # https://gitlab.kitware.com/vtk/vtk/-/issues/18098
    "\\.BoxWidget$"
    "\\.CheckSelectedBlocks$"
    "\\.CTHAMRClip$"
    "\\.CTHAMRContour$"
    "\\.MultiSliceWavelet$"
    "\\.NonConvexPolygon$"
    "\\.SelectCellsTrace$"
    "\\.SelectionLinkMultiple$"
    "\\.SelectionModifiersBlocks$"
    "\\.SpreadSheet1$"
    "\\.VariableSelector1$"
    "\\.VolumeCrop$"

    # "Structure does not match. You must use CopyStructure before calling this
    # method."
    # https://gitlab.kitware.com/paraview/paraview/-/issues/20692
    "\\.VisItBridgeLAMMPSDump$"
    "\\.VisItBridgeLAMMPSDump2$"

    # Invalid reading in parallel
    # https://gitlab.kitware.com/paraview/paraview/-/issues/20713
    "pvcs\\.VisItBridgeNek$"
    "pvcrs\\.VisItBridgeNek$"

    # These have failed on buildbot before. Not sure what the cause is.
    "\\.TestPythonView$"

    # X races
    # https://gitlab.kitware.com/paraview/paraview/-/issues/20697
    "\\.ComparativeViewOverlay"

    # UI tests that fail without messages or baselines.
    "\\.BagPlotMatrixView$"
    "\\.BagPlots$"
    "\\.BagPlotsSelection$"
    "\\.FunctionalBagPlots$"
    "\\.GMVReaderBasicRead$"
    "\\.LagrangianParticleTracker$"
    "\\.LagrangianParticleTrackerReseeding$"
    "\\.Slice$"
    "\\.TraceMultiViews$"
    "\\.TruchasReaderWithVisItBridge$"

    # Some X sync issue causing the images to be capture with
    # incorrect size, ignore for now.
    "\\.MultiSliceMultiBlock$"
    )
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos")
  list(APPEND test_exclusions
    # Segfaults in an event handler
    "\\.ColorOpacityTableEditing$"

    # Possibly https://gitlab.kitware.com/paraview/paraview/-/issues/19091
    "\\.SeparateOpacityArray$"

    # Known-bad
    "\\.SliceWithPlaneMultiBlock$"
    "\\.PreviewFontScaling$"

    # https://gitlab.kitware.com/paraview/paraview/-/issues/20594
    "\\.SelectCellsTrace$"
    "\\.SelectPointsTrace$"

    # Unstructured grid volume rendering (paraview/paraview#19130)
    "\\.MultiBlockVolumeRendering$"
    "\\.UnstructuredVolumeRenderingVectorComponent$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_arm64")
  list(APPEND test_exclusions
    # https://gitlab.kitware.com/paraview/paraview/-/issues/20743
    "^pv\\.ExtrusionRepresentationCellData$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  list(APPEND test_exclusions
    # Known-bad https://gitlab.kitware.com/paraview/paraview/-/issues/17433
    "^pv\\.ServerConnectDialog$"
    # Supposed to become obsolete eventually. Known to be flaky.
    "^pv\\.FindWidget$"

    # See https://gitlab.kitware.com/paraview/paraview/-/issues/20282
    "\\.AnimationSetTimeCursor$"
    "\\.ColorByCellDataStringArray$"
    "\\.IndexedLookupInitialization$"
    "\\.LoadStateMultiView$"
    "\\.PreviewFontScaling$"
    "\\.SaveColorMap$"
    "\\.SelectPointsTrace$"
    "\\.UndoRedo1$"
    "\\.UnstructuredVolumeRenderingVectorComponent$"
    "^pv\\.AnalyzeReaderWriterPlugin$"
    "^pv\\.CompositeSurfaceSelection$"
    "^pv\\.ExportFilteredColumnsSpreadsheet$"
    "^pv\\.ExportSpreadsheetFormatting$"
    "^pv\\.SaveStateAndScreenshot$"
    "^pvcrs\\.FindDataDialog$"
    "^pvcs\\.ColorOpacityTableEditorHistogram$"
    "^pvcs\\.SplitViewTrace$"
    "^pvcs-tile-display\\.LinkCameraFromView-1x1$"

    # The generated paths are too long and don't work in MSVC.
    # See https://gitlab.kitware.com/paraview/paraview/-/issues/20589
    "^pv\\.TestDevelopmentInstall$"

    # This test has some weird rendering artifacts. It looks like the Intel
    # rendering bug, but our machines all use nVidia cards today.
    "^paraviewPython-TestColorHistogram$"

    # There are warnings about initialization order confusion, but there are
    # then GL context errors. Not sure if these are related.
    "^paraviewPython-TestCatalystClient$"

    # Fails on windows.
    "pqWidgetsHeaderViewCheckState"

    # Fails sporadically (paraview/paraview#20702)
    "^pv\\.TestPythonConsole$"

    # Fails consistently, needs debugging (paraview/paraview#20742)
    "^pv\\.PythonAlgorithmPlugin$"
    )
endif ()

if ("$ENV{CC}" STREQUAL "icc")
  list(APPEND test_exclusions
    # Known-bad https://gitlab.kitware.com/paraview/paraview/-/issues/20108
    "^ParaViewExample-Catalyst$")
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
  REPEAT UNTIL_PASS:3)
ctest_submit(PARTS Test)

if (test_result)
  message(FATAL_ERROR
    "Failed to test")
endif ()
