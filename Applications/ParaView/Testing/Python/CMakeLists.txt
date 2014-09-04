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
include(TestNumPy)
if (FOUND_NUMPY)
  set(PARAVIEW_PYTHON_ARGS
    --data=${PARAVIEW_TEST_OUTPUT_DATA_DIR}/can.ex2)
  # Add pvpython tests
  paraview_add_test_python(
    NO_DATA NO_VALID NO_OUTPUT NO_RT
    TestPythonAnnotationFilter.py
    TestPythonAnnotationFilterNoMerge.py
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
  TestPythonViewScript.py
  CinemaTest.py
)

# these tests could run safely in serial and
# in parallel.
set(PVBATCH_TESTS
  StructuredGridVolumeRendering.py
  )
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
else()
  # run the test serially
  paraview_add_test_pvbatch(
    JUST_VALID
    ${PVBATCH_TESTS}
    )
endif()

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

# Extend timeout for CinemaTest
set_tests_properties(pvpythonPython-CinemaTest PROPERTIES TIMEOUT 500)
