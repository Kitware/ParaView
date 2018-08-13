include(ParaViewTestingMacros)

# Check that matplotlib is available
include(FindPythonModules)
find_python_module(matplotlib matplotlib_found)

# Set variables to make the testing functions.
set(vtk-module pvpython)
set(${vtk-module}_TEST_LABELS PARAVIEW)

paraview_test_load_data(""
  can.ex2
  multicomb_0.vts
  )

# only enable TestPythonAnnotationFilter test if numpy is available
find_python_module(numpy numpy_found)
if (numpy_found)
  set(PARAVIEW_PYTHON_ARGS
    --data=${PARAVIEW_TEST_OUTPUT_DATA_DIR}/can.ex2)
  # Add pvpython tests
  paraview_add_test_python(
    NO_DATA NO_VALID NO_OUTPUT NO_RT
    TestPythonAnnotationFilter.py
    TestPythonAnnotationFilterNoMerge.py
    TestAnnotateAttributeData.py
    CinemaAERTest.py
    )
  paraview_add_test_python(
    NO_VALID NO_OUTPUT NO_RT
    AppendAttributes.py
    UnstructuredCellTypePythonCalculator.py
    )
  paraview_add_test_python(
    NO_VALID NO_RT
    FileSeriesWriterSubTimeSteps.py
    )
  set(PARAVIEW_PYTHON_ARGS)

  if (PARAVIEW_ENABLE_MATPLOTLIB AND matplotlib_found)
    # add Matplotlib tests only if matplotlib was found at configure time.
    paraview_add_test_python(
      NO_DATA NO_RT
      TestPythonViewMatplotlibScript.py
      )
  endif()

  paraview_add_test_python(
    NO_DATA NO_RT
    TestPythonViewNumpyScript.py
    )
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
  TestResetProperty.py
  PointPicking.py
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
if (PARAVIEW_USE_MPI AND VTK_MPIRUN_EXE)
  # run the tests in parallel
  set(${vtk-module}_NUMPROCS 3)
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
  set(vtk-module multi-servers)
  set(TestMultiServersConfig_ARGS
    --test-multi-servers 2
    )
  set(TestMultiServersRemoteProxy_ARGS
    --test-multi-servers 3
    )
  set(${vtk-module}_TEST_LABELS PARAVIEW)

  paraview_add_test_driven(
    NO_DATA NO_VALID NO_OUTPUT NO_RT
    TestMultiServersConfig.py
    TestMultiServersRemoteProxy.py
    TestRemoteProgrammableFilter.py
    )
endif()
