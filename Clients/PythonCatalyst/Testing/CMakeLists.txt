#--------------------------------------------------------------------
# Process old tests before Catalyst refactoring.
add_subdirectory(Legacy)

#---------------------------------------------------------------------
# New testing stuff is here
vtk_module_test_data(

  # baselines
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletHistogramExtracts_ImmediateExtracts_renderview_0000.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletHistogramExtracts_BatchExtracts_histogram_1.000000.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletHistogramExtracts_CoprocessingExtracts_renderview_0000.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletHistogramExtracts_BatchExtracts_histogram_0.000000.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletHistogramExtracts_ImmediateExtracts_histogram_1.000000.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletHistogramExtracts_ImmediateExtracts_renderview_0008.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletHistogramExtracts_CoprocessingExtracts_histogram_0.950000.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletHistogramExtracts_ImmediateExtracts_histogram_0.000000.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletHistogramExtracts_BatchExtracts_renderview_0000.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletHistogramExtracts_BatchExtracts_renderview_0008.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletHistogramExtracts_CoprocessingExtracts_histogram_0.000000.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletHistogramExtracts_CoprocessingExtracts_renderview_0018.png"

  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/CinemaSpecAExtracts_BatchExtracts_cinema.cdb_8.888889e_01.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/CinemaSpecAExtracts_CoprocessingExtracts_cinema.cdb_9.500000e_01.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/CinemaSpecAExtracts_ImmediateExtracts_cinema.cdb_8.888889e_01.png"

  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/MultiplePipelines_Pipeline1_pipeline1_000000.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/MultiplePipelines_Pipeline2_pipeline2_000001.png"
)

set(simple_scripts
  ValidateChangingInput.py
  LiveOnly.py)

paraview_add_test(
  PREFIX "Catalyst::WaveletMiniApp"
  ARGS   "$<TARGET_FILE:ParaView::pvbatch>" -sym -m paraview.demos.wavelet_miniapp -t 20 --size 21 --script-version 2 -s
  TEST_SCRIPTS ${simple_scripts})
paraview_add_test_mpi(
  PREFIX "Catalyst::MPI::WaveletMiniApp"
  ARGS   "$<TARGET_FILE:ParaView::pvbatch>" -sym -m paraview.demos.wavelet_miniapp -t 20 --size 21 --script-version 2 -s
  TEST_SCRIPTS ${simple_scripts})

#--------------------------------------------------------------------
# add test to validate multiple analysis pipelines can be used.
paraview_add_test(
    PREFIX "Catalyst::WaveletMiniApp::MultiplePipelines::pipeline1"
    ENVIRONMENT
      PIPELINE1_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/Testing/Temporary/MultiplePipelines/Pipeline1
      PIPELINE2_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/Testing/Temporary/MultiplePipelines/Pipeline2
    ARGS   "$<TARGET_FILE:ParaView::pvbatch>" -sym -m
           paraview.demos.wavelet_miniapp -t 2 --size 21
           --script-version 2
           --channel Wavelet1
           -s "${CMAKE_CURRENT_SOURCE_DIR}/pipeline1.zip"
           -s
    TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline2.zip")

paraview_add_test(
    PREFIX "Catalyst::WaveletMiniApp::MultiplePipelines::Validate::pipeline1"
    ARGS   "$<TARGET_FILE:ParaView::pvpython>"
           -m paraview.tests.validate_extracts
           --name MultiplePipelines
           --root "${CMAKE_BINARY_DIR}/Testing/Temporary/MultiplePipelines"
           --baseline-dir "${paraview_test_data_directory_output}/Clients/PythonCatalyst/Testing/Data/Baseline"
           --temp-dir "${CMAKE_BINARY_DIR}/Testing/Temporary"
           --json
    TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline2.json")
set_tests_properties(
    "Catalyst::WaveletMiniApp::MultiplePipelines::Validate::pipeline1.pipeline2"
    PROPERTIES DEPENDS
      "Catalyst::WaveletMiniApp::MultiplePipelines::pipeline1.pipeline2")


#---------------------------------------------------------------------
# Let's add some full workflow tests.
# These workflow tests have several steps and require special care when
# creation. The following describes the steps involved.
#
# 1. Create the test XML: record a ParaView UI/XML test as usual with actions to
#    add extract generators (which are necessary).
#
#    One of the steps in the test must be generation of extracts using
#    "General Extracts" action. Choose output directories such that all extracts
#    are generated under the `$PARAVIEW_TEST_ROOT/ImmediateExtracts`.
#
#    Another step is save out a pvsm state file named same as the name of the
#    test in the `$PARAVIEW_TEST_ROOT` directory.
#
# 2. Once the XML test script is available, add to the `catalyst_tests`
#    variable. This will add a set of dependent tests. The tests are prefixed
#    with "Catalyst::Workflow::${stage}` where `stage` identifies the specific
#    tasks that the test does; and are suffixed with `.${testname}` where
#    `testname` is same the name of the test XML.
#
#    The various testing stages are:
#    * RemoveOld: cleans up output directory
#    * Prepare: creates an output directory `${PARAVIEW_TEST_ROOT}/${testname}`.
#      This is directory under which all output for this test are generated.
#    * ParaView: runs the XML test to generate extracts under
#      `ImmediateExtracts` dir and save out pvsm state file with same name as
#      the test.
#    * SaveCatalystState: loads the pvsm state file in pvpython and saves out a
#      Catalyst Python state archive.
#    * BatchExecute: loads the Catalyst Python state archive in pvbatch to
#      generate extracts in batch-mode under `${PARAVIEW_TEST_ROOT}/BatchExtracts`.
#    * WaveletMiniApp: runs the WaveletMiniApp with the Catalyst Python state
#      archive to generate extracts under `${PARAVIEW_TEST_ROOT}/CoprocessingExtracts`.
#
#    The various validation stages are:
#    * ParaView::Validate: validates results from Generate Extracts in the GUI.
#    * BatchExecute::Validate: validates results from batch execution
#    * WaveletMiniApp::Validate: validates results from miniapp execution.
#
# 3. To validate extracts, we use two parts. First, we use a listing of the
#    results directory to compare with the the test run. This is named as
#    `${testname}.json`. To generate this listing, run the test series to
#    generate all extracts in GUI, batch and coprocessing mode. Once generated,
#    on a unix system, run the following command to produce the json file.
#
#       tree --noreport -J ${PARAVIEW_TEST_ROOT}/${tname} -o ${CMAKE_CURRENT_SOURCE_DIR}/${tname}.json
#
#    The json can be editied to add `"compare"=true` attribute to images that
#    you want to do baseline comparisons for. Pick however many you need to
#    ensure correct results are being generated in all 3 forms for extract
#    generation.
#
# 4. To generate / add baselines, once the json file has been generated, re-run
#    the test sequence. This will generate appropriately named baselines under the
#    `${PARAVIEW_TEST_ROOT}` directory. Add them to source and the update the
#    `vtk_module_test_data` call at the start of this file to include the new
#    baselines.
#---------------------------------------------------------------------
if (PARAVIEW_USE_QT)
  set(catalyst_tests
    CinemaSpecAExtracts
    WaveletHistogramExtracts)

  foreach (tname IN LISTS catalyst_tests)
    set(ttemp_dir "${CMAKE_BINARY_DIR}/Testing/Temporary/${tname}")

    #----------------------------------------------------------------------
    # Fixture setup
    # remove ttemp_dir as first step
    add_test(NAME Catalyst::Workflow::RemoveOld.${tname}
             COMMAND "${CMAKE_COMMAND}" -E remove_directory "${ttemp_dir}")

    # create the ttemp_dir
    add_test(NAME Catalyst::Workflow::Prepare.${tname}
             COMMAND "${CMAKE_COMMAND}" -E make_directory "${ttemp_dir}")

    set_tests_properties(
        "Catalyst::Workflow::Prepare.${tname}"
      PROPERTIES
        FIXTURES_SETUP "CatalystWorkflowSetup.${tname}"
        FIXTURES_REQUIRED "CatalystWorkflowPreSetup.${tname}")

    set_tests_properties(
      "Catalyst::Workflow::RemoveOld.${tname}"
      PROPERTIES
        FIXTURES_SETUP "CatalystWorkflowPreSetup.${tname}")
    #----------------------------------------------------------------------

    # 1. run GUI to generate the pvsm state file and validate "export now"
    paraview_add_client_tests(
      PREFIX "Catalyst::Workflow::ParaView"
      TEST_DIRECTORY "${ttemp_dir}"
      TEST_SCRIPTS ${tname}.xml)

    set_tests_properties("Catalyst::Workflow::ParaView.${tname}"
      PROPERTIES
        FIXTURES_REQUIRED "CatalystWorkflowSetup.${tname}")

    # 1.1 validate 'ImmediateExtracts'
    paraview_add_test(
      PREFIX "Catalyst::Workflow::ParaView::Validate"
      ARGS "$<TARGET_FILE:ParaView::pvpython>"
           -m paraview.tests.validate_extracts
           --name "${tname}"
           --root  "${ttemp_dir}"
           --subdir "ImmediateExtracts"
           --baseline-dir "${paraview_test_data_directory_output}/Clients/PythonCatalyst/Testing/Data/Baseline"
           --temp-dir "${CMAKE_BINARY_DIR}/Testing/Temporary"
           --json
      TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/${tname}.json")

    set_tests_properties(
      "Catalyst::Workflow::ParaView::Validate.${tname}"
      PROPERTIES DEPENDS "Catalyst::Workflow::ParaView.${tname}")

    # 2. add test to generate Catalyst Python state. We'll just use pvpython for
    #    this.
    paraview_add_test(
      PREFIX "Catalyst::Workflow::SaveCatalystState"
      ARGS "$<TARGET_FILE:ParaView::pvpython>"
            -m paraview.demos.export_catalyst_state
            --output "${ttemp_dir}/${tname}.zip"
            --extracts-dir "${ttemp_dir}/CoprocessingExtracts"
            --pvsm
      TEST_SCRIPTS ${ttemp_dir}/${tname}.pvsm)

    set_tests_properties(
      "Catalyst::Workflow::SaveCatalystState.${tname}"
      PROPERTIES DEPENDS "Catalyst::Workflow::ParaView.${tname}")

    # 3. add test to load catalyst script in pvbatch.
    paraview_add_test(
      PREFIX "Catalyst::Workflow::BatchExecute"
      ENVIRONMENT
        PARAVIEW_OVERRIDE_DATA_OUTPUT_DIRECTORY=${ttemp_dir}/BatchExtracts
        PARAVIEW_OVERRIDE_IMAGE_OUTPUT_DIRECTORY=${ttemp_dir}/BatchExtracts
      ARGS "$<TARGET_FILE:ParaView::pvbatch>"
      TEST_SCRIPTS ${ttemp_dir}/${tname}.zip)

    set_tests_properties(
      "Catalyst::Workflow::BatchExecute.${tname}"
      PROPERTIES DEPENDS
        "Catalyst::Workflow::SaveCatalystState.${tname}")

    # 3.1 validate 'BatchExtracts'
    paraview_add_test(
      PREFIX "Catalyst::Workflow::BatchExecute::Validate"
      ARGS "$<TARGET_FILE:ParaView::pvpython>"
           -m paraview.tests.validate_extracts
           --name "${tname}"
           --root  "${ttemp_dir}"
           --subdir "BatchExtracts"
           --baseline-dir "${paraview_test_data_directory_output}/Clients/PythonCatalyst/Testing/Data/Baseline"
           --temp-dir "${CMAKE_BINARY_DIR}/Testing/Temporary"
           --json
      TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/${tname}.json")

    set_tests_properties(
      "Catalyst::Workflow::BatchExecute::Validate.${tname}"
      PROPERTIES DEPENDS "Catalyst::Workflow::BatchExecute.${tname}")

    # 4. run with WaveletMiniApp
    paraview_add_test(
      PREFIX "Catalyst::Workflow::WaveletMiniApp"
      ARGS "$<TARGET_FILE:ParaView::pvbatch>"
            -sym -m paraview.demos.wavelet_miniapp -t 20 --size 21
            --channel Wavelet1
            --script-version 2 -s
      TEST_SCRIPTS ${ttemp_dir}/${tname}.zip)

    set_tests_properties(
      "Catalyst::Workflow::WaveletMiniApp.${tname}"
      PROPERTIES DEPENDS
        "Catalyst::Workflow::SaveCatalystState.${tname}")

    # 4.1 validate 'CoprocessingExtracts'
    paraview_add_test(
      PREFIX "Catalyst::Workflow::WaveletMiniApp::Validate"
      ARGS "$<TARGET_FILE:ParaView::pvpython>"
           -m paraview.tests.validate_extracts
           --name "${tname}"
           --root  "${ttemp_dir}"
           --subdir "CoprocessingExtracts"
           --baseline-dir "${paraview_test_data_directory_output}/Clients/PythonCatalyst/Testing/Data/Baseline"
           --temp-dir "${CMAKE_BINARY_DIR}/Testing/Temporary"
           --json
      TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/${tname}.json")

    set_tests_properties(
      "Catalyst::Workflow::WaveletMiniApp::Validate.${tname}"
      PROPERTIES DEPENDS "Catalyst::Workflow::WaveletMiniApp.${tname}")
  endforeach()
else()
  message(STATUS "ParaView client not built. Skipping Catalyst workflow tests.")
endif()
