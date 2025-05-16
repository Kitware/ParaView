# Check that matplotlib is available
include(ParaViewFindPythonModules)
find_python_module(matplotlib matplotlib_found)

# Set variables to make the testing functions.
set(_vtk_build_test "paraview")
set(${_vtk_build_test}_TEST_LABELS paraview)

ExternalData_Expand_Arguments(ParaViewData _
  # data files
  "DATA{${paraview_test_data_directory_input}/Data/BackwardCompatibility/LegacyPathLine.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/BackwardCompatibility/LegacyStreakLine.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/vtk_dark_bkg.png}"
  "DATA{${paraview_test_data_directory_input}/Data/blow.vtk}"
  "DATA{${paraview_test_data_directory_input}/Data/can.ex2}"
  "DATA{${paraview_test_data_directory_input}/Data/iron_protein.vtk}"
  "DATA{${paraview_test_data_directory_input}/Data/multicomb_0.vts}"
  "DATA{${paraview_test_data_directory_input}/Data/mg_diff_0000.vtm}"
  "DATA{${paraview_test_data_directory_input}/Data/mg_diff_0000/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/mg_diff_0062.vtm}"
  "DATA{${paraview_test_data_directory_input}/Data/mg_diff_0062/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/glTF/NestedRings/NestedRings.glb}"
  "DATA{${paraview_test_data_directory_input}/Data/plotly_json_line_chart.json}"
  "DATA{${paraview_test_data_directory_input}/Data/WorldWithTexture.obj}"
  "DATA{${paraview_test_data_directory_input}/Data/WorldWithTexture.png}"


  # baselines
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestPythonViewMatplotlibScript.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestHTGContourVoxelsPolyhedron.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestHTG3DContourPolyhedron.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestHTGContourMonoHT.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestImportGLTFNestedScene.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestPythonViewNumpyScript.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestCatalystClient.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/FontSize.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SaveTransparentImages.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LoadAndApplyTexture.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/LoadStateWithTextures.png}"
  )

# only enable TestPythonAnnotationFilter test if numpy is available
find_python_module(numpy numpy_found)
if (numpy_found)
  paraview_add_test_python(
    NO_DATA NO_VALID NO_RT
    TestAnnotateAttributeData.py
    )

  paraview_add_test_python(
    NO_DATA NO_RT
    TestPythonViewNumpyScript.py
    )

  paraview_add_test_python(
    NO_VALID NO_RT
    AppendAttributes.py
    FileSeriesWriterSubTimeSteps.py
    TestPlotlyJsonExtractor.py
    TestPythonAnnotationFilterNoMerge.py
    TestPythonAnnotationFilter.py
    UnstructuredCellTypePythonCalculator.py
    )

  if (matplotlib_found)
    # add Matplotlib tests only if matplotlib was found at configure time.
    paraview_add_test_python(
      NO_DATA NO_RT
      TestPythonViewMatplotlibScript.py,LOOSE_VALID
      )
  endif()
endif()

paraview_add_test_pvbatch(
  NO_DATA NO_RT

  # Adding as a pvbatch test (i.e. use offscreen)
  # since the test may fail on X due to X window size sync issues
  # on certain dashboards
  OutputImageResolution.py
)

paraview_add_test_python(
  NO_RT
  ColorBlockBy.py
  ColorBy.py
  CtxActiveProxy.py
  Deprecation.py
  GroupDataSets.py
  LegacyPathLine.py
  LegacyStreakLine.py
  PartialArrayIndication.py
  ResetSession.py
  SaveTransparentImages.py
  TestClipCylinder.py
  TestColorHistogram.py
  TestGeometryBoundsClobber.py
  TestHoudiniWriter.py
  TestImportGLTFNestedScene.py
  TestLineChartLabelPrefix.py
  TestPythonViewScript.py
  TestResetCameraToDirection.py
  LoadAndApplyTexture.py
  LoadStateWithTextures.py
  ExportGLTF.py
)

if(NOT APPLE)
  # PvPython test is not added on mac because of
  # https://gitlab.kitware.com/paraview/paraview/-/issues/20652
  paraview_add_test_python(
    NO_RT
    PvPython.py)
endif()

paraview_add_test_python(
  NO_DATA NO_VALID NO_OUTPUT NO_RT
  AppendDatasetsPolyData.py
  MergeBlocksPolyData.py
  PointPicking.py
  TestGhostCellsGenerator.py
  TestResetProperty.py
  TestOverlappingCellsDetector.py
)

paraview_add_test_python(
  NO_VALID NO_RT
  ExportCSV.py
  LoadStateWithOptions.py
  LoadStateWithDataSets.py
  Plot3DReader.py
  SaveDataWithArraySelection.py
  SaveDataWithArrayAndBlockSelection.py
  ReadTimeArray.py
  ReplaceReaderFileName.py
  ReplaceReaderFileNameNoExtensions.py
)

# These tests could run safely in serial and parallel.
set(PVBATCH_TESTS
  BatchAxesGrid.py
  TestXYChartOpacity.py
  FontSize.py # BUG #19816
  PLYWriter.py
  VolumeCellSelection.py
  SaveDataWithArraySelection.py,NO_VALID
  NoParaViewInit.py,NO_VALID
  # SaveDataWithArrayAndBlockSelection.py,NO_VALID # BUG #21419
)
if(NOT APPLE)
  # temporarily disable on mac due to bugs in
  # ProjectedTetraHedraMapper (paraview/paraview#17620).
  list(APPEND PVBATCH_TESTS
    StructuredGridVolumeRendering.py)
endif()

foreach (tfile IN LISTS PVBATCH_TESTS)
  get_filename_component(test ${tfile} NAME_WE)
  set(${test}_ARGS
    --state ${CMAKE_CURRENT_SOURCE_DIR}/${test}.pvsm)
endforeach ()
if (PARAVIEW_USE_MPI AND MPIEXEC_EXECUTABLE)
  # run the tests in parallel
  set(_NUMPROCS 3)
  paraview_add_test_pvbatch_mpi(
    JUST_VALID
    ${PVBATCH_TESTS}
    )

  if (numpy_found)
    paraview_add_test_pvbatch_mpi(
      NO_DATA NO_VALID NO_OUTPUT NO_RT
      TestAnnotateAttributeData.py
      )
  endif()
else()
  # run the test serially
  paraview_add_test_pvbatch(
    JUST_VALID
    ${PVBATCH_TESTS}
    )
  if (numpy_found)
    paraview_add_test_pvbatch(
      NO_DATA NO_VALID NO_OUTPUT NO_RT
      TestAnnotateAttributeData.py
      )
  endif()
endif()


paraview_add_test_driven(
  JUST_VALID
  TestCompositedGeometryCulling.py
)

# Python Multi-servers test
# => Only for shared build as we dynamically load plugins
if(BUILD_SHARED_LIBS)
  set(TestMultiServersConfig_ARGS
    --test-multi-servers 2
    )
  set(TestMultiServersRemoteProxy_ARGS
    --test-multi-servers 3
    )
  set(${_vtk_build_test}_TEST_LABELS paraview)

  paraview_add_test_driven(
    NO_DATA NO_VALID NO_OUTPUT NO_RT
    TestMultiServersConfig.py
    TestMultiServersRemoteProxy.py
    TestRemoteProgrammableFilter.py
    )
endif()

#------------------------------------------------------------------------------
# add a test for stereo batch (#18974).
set(paraview_pvbatch_args --stereo --stereo-type=SplitViewportHorizontal)
paraview_add_test_pvbatch(
  NO_DATA NO_RT
  TestStereoSaveScreenshot.py
)
if(PARAVIEW_USE_MPI)
  paraview_add_test_pvbatch_mpi(
    NO_DATA NO_RT
    TestStereoSaveScreenshot.py
  )
endif()
unset(paraview_pvbatch_args)

set(paraview_python_args --stereo --stereo-type=SplitViewportHorizontal)
paraview_add_test_python(
  NO_DATA NO_RT
  TestStereoSaveScreenshot.py
)
unset(paraview_python_args)

#------------------------------------------------------------------------------
if (PARAVIEW_USE_MPI AND MPIEXEC_EXECUTABLE)
  set(_vtk_testing_python_exe "$<TARGET_FILE:ParaView::smTestDriver>")
  set(_vtk_test_python_args
    --script $<TARGET_FILE:ParaView::pvbatch> --sym
      -m paraview.demos.wavelet_miniapp
      -s ${CMAKE_CURRENT_SOURCE_DIR}/DoLive.py
      -t 10
      --size 21
      --delay 2
    --client $<TARGET_FILE:ParaView::pvpython> --force-offscreen-rendering -dr)
  vtk_add_test_python(NO_DATA NO_RT TestCatalystClient.py)
  unset(_vtk_testing_python_exe)
  unset(_vtk_test_python_args)
endif()

#------------------------------------------------------------------------------
# Test pvx file configuration is available to the client
if (PARAVIEW_USE_MPI AND MPIEXEC_EXECUTABLE AND TARGET pvserver AND TARGET pvpython)
  set(num_procs 2)

  _paraview_add_tests("paraview_add_cave_python_tests"
    PREFIX "pv_cave_python"
    SUFFIX "-${num_procs}"
    ENVIRONMENT
      PV_SHARED_WINDOW_SIZE=400x300
      SMTESTDRIVER_MPI_NUMPROCS=${num_procs}
    NUMPROCS "${num_procs}"
    _COMMAND_PATTERN
      __paraview_smtesting_args__
      --server "$<TARGET_FILE:ParaView::pvserver>"
        --enable-bt
        --force-offscreen-rendering
        --pvx
        "${CMAKE_CURRENT_SOURCE_DIR}/../XML/LeftRight.pvx"
      --client $<TARGET_FILE:ParaView::pvpython>
        --enable-bt
        --dr
        "${CMAKE_CURRENT_SOURCE_DIR}/TestPVXInfoAvailableInClient.py"
    TEST_NAME "TestPVXInfoAvailableInClient")
endif()

#------------------------------------------------------------------------------
# Add a test using python plugin and a python script

paraview_add_test_python(
  NO_DATA NO_RT
  TestPV_PLUGIN_PATH.py
)

set_tests_properties(paraviewPython-TestPV_PLUGIN_PATH
        PROPERTIES ENVIRONMENT "PV_PLUGIN_PATH=${CMAKE_SOURCE_DIR}/Examples/Plugins/PythonAlgorithm/Plugin")

#------------------------------------------------------------------------------
# Add a couple of tests for pvpython package listing. Just verify they run.
add_test(NAME pvpython-ListPackagesTest
  COMMAND
    "$<TARGET_FILE:ParaView::pvpython>"
      -m paraview.apps.packages)

add_test(NAME pvpython-ListPackagesRequirementsTest
  COMMAND
    "$<TARGET_FILE:ParaView::pvpython>"
      -m paraview.apps.packages
      --format requirements)

#------------------------------------------------------------------------------
# Add HTG rendering tests
paraview_add_test_python(
  NO_DATA NO_RT
  TestHTGContourVoxelsPolyhedron.py
  TestHTG3DContourPolyhedron.py
  TestHTGContourMonoHT.py
)
