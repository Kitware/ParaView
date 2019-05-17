# Check that matplotlib is available
include(FindPythonModules)
find_python_module(matplotlib matplotlib_found)

# Set variables to make the testing functions.
set(_vtk_build_test "paraview")
set(${_vtk_build_test}_TEST_LABELS paraview)

ExternalData_Expand_Arguments(ParaViewData _
  # data files
  "DATA{${paraview_test_data_directory_input}/Data/can.ex2}"
  "DATA{${paraview_test_data_directory_input}/Data/multicomb_0.vts}"

  # baselines
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestPythonViewMatplotlibScript.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestPythonViewNumpyScript.png}"
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
    TestPythonAnnotationFilterNoMerge.py
    TestPythonAnnotationFilter.py
    UnstructuredCellTypePythonCalculator.py
    )

  if (matplotlib_found)
    # add Matplotlib tests only if matplotlib was found at configure time.
    paraview_add_test_python(
      NO_DATA NO_RT
      TestPythonViewMatplotlibScript.py
      )
  endif()
endif()

paraview_add_test_python(
  NO_DATA NO_RT
  ColorBy.py
  OutputImageResolution.py
  PartialArrayIndication.py
  ResetSession.py
  TestClipCylinder.py
  TestColorHistogram.py
  TestGeometryBoundsClobber.py
  TestHoudiniWriter.py
  TestLineChartLabelPrefix.py
  TestPythonViewScript.py
)

paraview_add_test_python(
  NO_DATA NO_VALID NO_OUTPUT NO_RT
  AppendDatasetsPolyData.py
  MergeBlocksPolyData.py
  PointPicking.py
  TestResetProperty.py
)

paraview_add_test_python(
  NO_VALID NO_RT
  ExportCSV.py
  Plot3DReader.py
)

# These tests could run safely in serial and parallel.
set(PVBATCH_TESTS
  BatchAxesGrid.py
  PLYWriter.py
  VolumeCellSelection.py
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
