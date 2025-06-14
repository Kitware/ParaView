###############################################################################
# For python scripts for testing.
#
# These tests depend on target ParaView::RemotingMisc, which may not
# exist in the edition being built. Exit
if (NOT TARGET ParaView::RemotingMisc)
  return()
endif ()

vtk_module_test_data(
  "${paraview_test_data_directory_input}/Data/StateWithHiddenRepresentations.pvsm")

# Add python script names here.
set(PY_TESTS
  Animation.py
  AnimationCache.py,NO_VALID
  AxesGridTestGridLines.py
  BackgroundColorBackwardsCompatibilityTest.py,NO_VALID
  CellIntegrator.py,NO_VALID
  ChangeTimeSteps.py
  ColorAttributeTypeBackwardsCompatibility.py,NO_VALID
  ColorPaletteInStateFile.py
  CompositeDataFieldArraysInformation.py,NO_VALID
  ConnectionProxyNamespaces.py,NO_VALID
  CSVWriterReader.py,NO_VALID
  FailingRequestDataObject.py,NO_VALID
  GenerateIdScalarsBackwardsCompatibility.py,NO_VALID
  GetActiveCamera.py,NO_VALID
  GhostCellsInMergeBlocks.py
  HTGCellCentersBackwardsCompatibilityTest.py,NO_VALID
  HTGFeatureEdgesBackwardsCompatibilityTest.py,NO_VALID
  HTGGhostCellsGeneratorBackwardsCompatibilityTest.py,NO_VALID
  HTGReflectionFilterBackwardsCompatibilityTest.py,NO_VALID
  HTGVisibleLeavesSizeBackwardsCompatibilityTest.py,NO_VALID
  IntegrateAttributes.py,NO_VALID
  LocatorNonMergingBackwardsCompatibilityTest.py,NO_VALID
  LookupTable.py,NO_VALID
  MultiServer.py,NO_VALID
  PlotOverLineLeaks.py,NO_VALID
  PointGaussianProperties.py
  ProcessIdScalarsBackwardsCompatibilityTest.py,NO_VALID
  ProgrammableFilterProperties.py,NO_VALID
  ProgrammableFilter.py,NO_VALID
  ProxyManager.py,NO_VALID
  ProxyPropertyLinks.py
  PythonVersion.py,NO_VALID
  PythonAnimationTrack.py
  PythonInfoModule.py,NO_VALID
  PythonProgrammableFilterParameters.py,NO_VALID
  PythonPVSimpleCone.py
  PythonPVSimpleExII.py
  PythonPVSimpleSphere.py
  PythonSMTraceTest1.py
  PythonSMTraceTest2.py,NO_VALID
  PythonSMTraceTest4.py,NO_VALID
  PythonSMTraceTest5.py,NO_VALID
  PythonSMTraceTest6.py,NO_VALID
  PythonSMTraceTest7.py,NO_VALID
  PythonSMScopedTrace.py,NO_VALID
  PythonDecorators.py,NO_VALID
  PythonTestBenchmark.py,NO_VALID
  ReaderReload.py,NO_VALID
  RecolorableImageExtractor.py
  ReflectBackwardsCompatibilityTest.py,NO_VALID
  RepresentationTypeHint.py,NO_VALID
  SaveAnimation.py
  SaveScreenshot.py,NO_VALID
  ScalarBarActorBackwardsCompatibility.py,NO_VALID
  SliceBackwardsCompatibilityTest.py,NO_VALID
  StateWithHiddenRepresentations.py,NO_VALID
  TestFetchData.py,NO_VALID
  TestSAVGReader.py
  TestVTKSeriesWithMeta.py
  ThresholdBackwardsCompatibilityTest.py,NO_VALID
  ValidateSources.py,NO_VALID
  VRMLSource.py,NO_VALID
  )

include(ParaViewFindPythonModules)
find_python_module(numpy numpy_found)
if (numpy_found)
  list(APPEND PY_TESTS PythonSelection.py PythonSMTraceTest3.py)
endif ()

if (PARAVIEW_PLUGIN_ENABLE_SurfaceLIC AND PARAVIEW_PLUGIN_ENABLE_Moments)
  list(APPEND PY_TESTS
    Plugins.py,NO_VALID)
endif ()

if (NOT BUILD_SHARED_LIBS)
  list(APPEND PY_TESTS
    ZIPImport.py,NO_VALID)
endif()

if (VTK_MODULE_ENABLE_VTK_IOAvmesh)
  list(APPEND PY_TESTS AvmeshReader.py,NO_VALID)
  vtk_module_test_data(
    Data/vwing_hexle.avm)
endif ()

set(SMSTATE_FILE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(ParaView::RemotingApplication_ARGS
  -S "${SMSTATE_FILE_DIR}")
paraview_add_test_python(
  JUST_VALID
  ${PY_TESTS}
  )
set(ParaView::RemotingApplication_ARGS)

###############################################################################
# Add tests for pvbatch.

vtk_module_test_data(
  "${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SaveAnimation_right.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MultiView_chart_view.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/MultiView_render_view.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/RecolorableImageExtractor_1.png")

set(PVBATCH_TESTS
  AnnotationVisibility.py
  LinePlotInScripts.py,NO_VALID
  MultiView.py
  ParallelImageWriter.py,NO_VALID
  ParallelSerialWriter.py
  ParallelSerialWriterWithIOSS.py
  PotentialMismatchedDataDelivery.py,NO_VALID
  SaveScreenshot.py,NO_VALID
  Simple.py
  TestFetchData.py,NO_VALID
  UserTransformOnRepresentation.py
  )

if (numpy_found AND PARAVIEW_USE_MPI)
  list(APPEND PVBATCH_TESTS
    D3CellsWithNegativeVolumes.py,NO_VALID)
endif()

if (PARAVIEW_PLUGIN_ENABLE_SurfaceLIC AND PARAVIEW_PLUGIN_ENABLE_Moments)
  list(APPEND PVBATCH_TESTS
    Plugins.py,NO_VALID)
endif ()

# Saving animation currently doesn't work in symmetric mode.
# paraview/paraview#17329
set(PVBATCH_NO_SYMMETRIC_TESTS
  SaveAnimation.py
  )

# Test tests require symmetric mode
set(PVBATCH_SYMMETRIC_TESTS
  RecolorableImageExtractor.py
  )

set(PVBATCH_TESTS_5_RANKS
  ParallelSerialWriterMultipleRankIO.py)

set(PVBATCH_TESTS_5_RANKS_NO_SYMMETRIC
  GatherRankSpecificDataInformation.py,NO_VALID)

IF (MPIEXEC_EXECUTABLE)
  set(vtkRemotingApplication_NUMPROCS 2)
  paraview_add_test_pvbatch_mpi(
    JUST_VALID
    ${PVBATCH_TESTS}
    ${PVBATCH_NO_SYMMETRIC_TESTS}
    )
  set(vtkRemotingApplication_NUMPROCS 5)
  paraview_add_test_pvbatch_mpi(
    JUST_VALID
    ${PVBATCH_TESTS_5_RANKS}
    ${PVBATCH_TESTS_5_RANKS_NO_SYMMETRIC}
    )

  set(vtkRemotingApplication_NUMPROCS 2)
  set(paraview_pvbatch_args
    --symmetric)
  set(vtk_test_prefix Symmetric)
  paraview_add_test_pvbatch_mpi(
    JUST_VALID
    ${PVBATCH_TESTS}
    ${PVBATCH_SYMMETRIC_TESTS}
    )

  set(vtkRemotingApplication_NUMPROCS 5)
  paraview_add_test_pvbatch_mpi(
    JUST_VALID
    ${PVBATCH_TESTS_5_RANKS}
    )

  unset(paraview_pvbatch_args)
  unset(vtk_test_prefix)
  unset(vtkRemotingApplication_NUMPROCS)
else ()
  paraview_add_test_pvbatch(
    JUST_VALID
    ${PVBATCH_TESTS}
    ${PVBATCH_NO_SYMMETRIC_TESTS}
    )
  set(paraview_pvbatch_args
    --symmetric)
  set(vtk_test_prefix Symmetric)
  paraview_add_test_pvbatch(
    JUST_VALID
    ${PVBATCH_TESTS}
    ${PVBATCH_SYMMETRIC_TESTS}
    )
  unset(paraview_pvbatch_args)
  unset(vtk_test_prefix)
endif ()

if (numpy_found)
  # This is necessary because PythonFilters generates and executes python scripts
  # on the fly which throws exceptions but cannot be caught by the host script
  # for some reason. Therefore, those exceptions are not considered as failures
  # by the ctest framework. Set this property causes ctest to parse the outputs of
  # the generated scripts for the regex "Error" which can cause a test failure.

  # The following lines are commented out to make sure PythonFilters-Batch does
  # not trash the DashBoard because it always fails. The solution, however, is
  # out of my control. When the related problem is fixed, please uncomment this
  # line.
  # set_tests_properties(vtkRemotingApplicationPython-MPI-PythonFilters PROPERTIES FAIL_REGULAR_EXPRESSION "Error")
  # set_tests_properties(vtkRemotingApplicationPython-MPI-SymmetricPythonFilters PROPERTIES FAIL_REGULAR_EXPRESSION "Error")
  # set_tests_properties(vtkRemotingApplicationPython-PythonFilters PROPERTIES FAIL_REGULAR_EXPRESSION "Error")
  # set_tests_properties(vtkRemotingApplicationPython-SymmetricPythonFilters PROPERTIES FAIL_REGULAR_EXPRESSION "Error")
endif()

if (PARAVIEW_USE_MPI AND MPIEXEC_EXECUTABLE AND NOT WIN32)
  set(paraview_pvbatch_args
    --symmetric)
  paraview_add_test_pvbatch_mpi(
    NO_DATA NO_OUTPUT NO_VALID
    TestMPI4PY.py
    ParallelPythonImport.py
    )
  unset(paraview_pvbatch_args)
endif()

# Python state tests. Each test executes an XML test in the ParaView UI, saves
# the state as a Python state file, runs the Python state file script in
# pvpython, then checks that the same image is generated in both the UI and
# pvpython. Each <TEST> list here requires a <TEST>.xml that generates the
# trace and a <TEST>.png file for comparison to a baseline.
set(paraview_python_state_tests
  SavePythonState
  ScalarOpacityFunctionPythonState
  SavePythonStateCameraAndTime
  SavePythonStateSliceOffset
)
foreach(python_state_test ${paraview_python_state_tests})
  if (TARGET ParaView::paraview OR paraview_will_be_built)
    set(tname "ParaView::RemotingApplicationPython-${python_state_test}")
    ExternalData_add_test("${_vtk_build_TEST_DATA_TARGET}" NAME ${tname}
      COMMAND ${CMAKE_COMMAND}
      -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::paraview>
      -DPVPYTHON_EXECUTABLE:FILEPATH=$<TARGET_FILE:ParaView::pvpython>
      -DPARAVIEW_TEST_OUTPUT_BASELINE_DIR:PATH=${paraview_test_data_directory_output}/Testing/Data/Baseline
      -DTEST_DRIVER:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/PythonStateTestDriver.py
      -DTEST_SCRIPT:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/${python_state_test}.xml
      -DTEMPORARY_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
      -DDATA_DIR:PATH=${paraview_test_data_directory_output}
      -DPYTHON_STATE_TEST_NAME:STRING=${python_state_test}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/PythonStateTestDriver.cmake
      DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/Baseline/${python_state_test}.png}
      )
    set_tests_properties(${tname} PROPERTIES LABELS "PARAVIEW")
  endif()
endforeach()

if (PYTHON_EXECUTABLE AND NOT PARAVIEW_BUILD_SHARED_LIBS)
  # For now, this test is only added for static builds.
  # In theory, we can make it work in shared builds too, it just needs more work to figure out
  # what variables need to be set on each platform correctly.
  add_test(NAME ParaView::RemotingApplicationPython-TestImportVanillaInterpreter
    COMMAND "${PYTHON_EXECUTABLE}" -m paraview.simple)
  set_tests_properties("ParaView::RemotingApplicationPython-TestImportVanillaInterpreter"
    PROPERTIES
      LABELS "${_vtk_build_test_labels}"
      FAIL_REGULAR_EXPRESSION "${_vtk_fail_regex}"
      # This must match VTK_SKIP_RETURN_CODE in vtkTestingObjectFactory.h
      SKIP_RETURN_CODE 125
      ENVIRONMENT
      "PYTHONPATH=${CMAKE_BINARY_DIR}/${PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX}"
    )
endif()

#----------------------------------------------------------------------
# Module tests using pvpython
#----------------------------------------------------------------------
if (NOT PARAVIEW_SERIAL_TESTS_USE_MPIEXEC)
  # for HPC, we only test pvbatch, and that's adequate.
  paraview_add_test(
    PREFIX "ParaView::RemotingApplicationPython::Module::pvpython::paraview"
    ARGS   "$<TARGET_FILE:ParaView::pvpython>"
           --dr
           --force-offscreen-rendering
           --
           -m paraview.tests
           -o "${CMAKE_BINARY_DIR}/Temporary/Temporary/paraview_tests_results"
    TEST_NAME "tests")

  paraview_add_test(
    PREFIX "ParaView::RemotingApplicationPython::Module::pvpython::paraview.tests"
    ARGS   "$<TARGET_FILE:ParaView::pvpython>"
           --dr
           --force-offscreen-rendering
           --
           -m paraview.tests.verify_eyedomelighting
           -o "${CMAKE_BINARY_DIR}/Testing/Temporary/VerifyEyeDomeLighting.png"
           -v DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/Baseline/VerifyEyeDomeLighting.png}
    TEST_NAME "verify_eyedomelighting")

  paraview_add_test(
    PREFIX "ParaView::RemotingApplicationPython::Module::pvpython::paraview.tests"
    ARGS   "$<TARGET_FILE:ParaView::pvpython>"
           --dr
           --force-offscreen-rendering
           --
           -m paraview.tests.basic_rendering
           -o "${CMAKE_BINARY_DIR}/Testing/Temporary/BasicRendering.png"
           -v DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/Baseline/BasicRendering.png}
    TEST_NAME "basic_rendering")

  paraview_add_test(
    PREFIX "ParaView::RemotingApplicationPython::Module::pvpython::paraview.tests"
    ARGS   "$<TARGET_FILE:ParaView::pvpython>"
           --dr
           --
           -m paraview.tests.proxy_deprecation
    TEST_NAME "proxy_deprecation")

  paraview_add_test(
          PREFIX "ParaView::RemotingApplicationPython::Module::pvpython::paraview.tests"
          ARGS   "$<TARGET_FILE:ParaView::pvpython>"
          --dr
          --
          -m paraview.tests.fetch_composite
          TEST_NAME "fetch_composite")

endif()

#----------------------------------------------------------------------
# Module tests using pvbatch (with or without MPI)
#----------------------------------------------------------------------
paraview_add_test_mpi_optional(
  PREFIX "ParaView::RemotingApplicationPython::Module::pvbatch::paraview"
  ARGS   "$<TARGET_FILE:ParaView::pvbatch>"
         --dr
         --
         -m paraview.tests
         -o "${CMAKE_BINARY_DIR}/Temporary/Temporary/paraview_tests_results"
  TEST_NAME "tests")

paraview_add_test_mpi_optional(
  PREFIX "ParaView::RemotingApplicationPython::Module::pvbatch::paraview.tests"
  ARGS   "$<TARGET_FILE:ParaView::pvbatch>"
         --dr
         --
         -m paraview.tests.verify_eyedomelighting
         -o "${CMAKE_BINARY_DIR}/Testing/Temporary/VerifyEyeDomeLighting.png"
         -v DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/Baseline/VerifyEyeDomeLighting.png}
  TEST_NAME "verify_eyedomelighting")

paraview_add_test_mpi_optional(
  PREFIX "ParaView::RemotingApplicationPython::Module::pvbatch::paraview.tests"
  ARGS   "$<TARGET_FILE:ParaView::pvbatch>"
         --dr
         --
         -m paraview.tests.basic_rendering
         -o "${CMAKE_BINARY_DIR}/Testing/Temporary/BasicRendering.png"
         -v DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/Baseline/BasicRendering.png}
  TEST_NAME "basic_rendering")

paraview_add_test_mpi_optional(
  PREFIX "ParaView::RemotingApplicationPython::Module::pvbatch::paraview.tests"
  ARGS   "$<TARGET_FILE:ParaView::pvbatch>"
         --dr
         --
         -m paraview.tests.proxy_deprecation
  TEST_NAME "proxy_deprecation")

#----------------------------------------------------------------------
# Module tests using pvbatch with MPI and in symmetric mode.
#----------------------------------------------------------------------
paraview_add_test_mpi(
  PREFIX "ParaView::RemotingApplicationPython::Module::pvbatch.sym::paraview"
  ARGS   "$<TARGET_FILE:ParaView::pvbatch>"
         --dr
         --sym
         --
         -m paraview.tests
         -o "${CMAKE_BINARY_DIR}/Temporary/Temporary/paraview_tests_results"
  TEST_NAME "tests")

paraview_add_test_mpi(
  PREFIX "ParaView::RemotingApplicationPython::Module::pvbatch.sym::paraview.tests"
  ARGS   "$<TARGET_FILE:ParaView::pvbatch>"
         --dr
         --sym
         --
         -m paraview.tests.verify_eyedomelighting
         -o "${CMAKE_BINARY_DIR}/Testing/Temporary/VerifyEyeDomeLighting.png"
         -v DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/Baseline/VerifyEyeDomeLighting.png}
  TEST_NAME "verify_eyedomelighting")

paraview_add_test_mpi(
  PREFIX "ParaView::RemotingApplicationPython::Module::pvbatch.sym::paraview.tests"
  ARGS   "$<TARGET_FILE:ParaView::pvbatch>"
         --dr
         --sym
         --
         -m paraview.tests.basic_rendering
         -o "${CMAKE_BINARY_DIR}/Testing/Temporary/BasicRendering.png"
         -v DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/Baseline/BasicRendering.png}
  TEST_NAME "basic_rendering")
