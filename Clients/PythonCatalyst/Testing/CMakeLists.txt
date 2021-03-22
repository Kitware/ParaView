#--------------------------------------------------------------------
# Process old tests before Catalyst refactoring.
add_subdirectory(Legacy)

#---------------------------------------------------------------------
# New testing stuff is here
vtk_module_test_data(

  # Data files
  "DATA{${paraview_test_data_directory_input}/Data/Wavelet/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/Wavelet/Wavelet1_000000/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/Wavelet/Wavelet1_000001/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/Wavelet/Wavelet1_000002/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/Wavelet/Wavelet1_000003/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/Wavelet/Wavelet1_000004/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/Wavelet/Wavelet1_000005/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/Wavelet/Wavelet1_000006/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/Wavelet/Wavelet1_000007/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/Wavelet/Wavelet1_000008/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/Wavelet/Wavelet1_000009/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/Wavelet/Wavelet1_000010/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/can_vtm/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/can_vtm/can_0/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/can_vtm/can_1/,REGEX:.*}"
  "DATA{${paraview_test_data_directory_input}/Data/can_vtm/can_2/,REGEX:.*}"

  # baselines for WaveletHistogramExtracts
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

  # baselines for WaveletHistogramExtracts with MPI. The histograms are difference since points along edges are counted twice.
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletHistogramExtracts_BatchExtracts_histogram_0.000000_1.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletHistogramExtracts_BatchExtracts_histogram_1.000000_1.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletHistogramExtracts_CoprocessingExtracts_histogram_0.000000_1.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletHistogramExtracts_CoprocessingExtracts_histogram_0.950000_1.png"

  # baselines for WaveletPhiThetaExtractsWithCinema
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletPhiThetaExtractsWithCinema_BatchExtracts_image_000000p_000.00t_000.00.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletPhiThetaExtractsWithCinema_BatchExtracts_image_000000p_000.00t_060.00.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletPhiThetaExtractsWithCinema_BatchExtracts_image_000001p_000.00t_180.00.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletPhiThetaExtractsWithCinema_BatchExtracts_image_000001p_000.00t_240.00.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletPhiThetaExtractsWithCinema_CoprocessingExtracts_image_000000p_000.00t_000.00.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletPhiThetaExtractsWithCinema_CoprocessingExtracts_image_000000p_000.00t_060.00.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletPhiThetaExtractsWithCinema_CoprocessingExtracts_image_000001p_000.00t_180.00.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletPhiThetaExtractsWithCinema_CoprocessingExtracts_image_000001p_000.00t_240.00.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletPhiThetaExtractsWithCinema_ImmediateExtracts_image_000000p_000.00t_000.00.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletPhiThetaExtractsWithCinema_ImmediateExtracts_image_000000p_000.00t_060.00.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletPhiThetaExtractsWithCinema_ImmediateExtracts_image_000001p_000.00t_000.00.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletPhiThetaExtractsWithCinema_ImmediateExtracts_image_000001p_000.00t_060.00.png"

  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/MultiplePipelines_pipeline1_000001.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/MultiplePipelines_pipeline2_000009.png"

  # baselines for WaveletVolumeRenderWithCinema
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletVolumeRenderWithCinema_BatchExtracts_image_000000otf_0.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletVolumeRenderWithCinema_BatchExtracts_image_000000otf_4.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletVolumeRenderWithCinema_CoprocessingExtracts_image_000000otf_0.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletVolumeRenderWithCinema_CoprocessingExtracts_image_000000otf_4.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletVolumeRenderWithCinema_ImmediateExtracts_image_000000otf_0.png"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletVolumeRenderWithCinema_ImmediateExtracts_image_000000otf_4.png"

  # baselines for WaveletTimeTrigger
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletTimeTrigger_BatchExtracts_RenderView1_000000.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletTimeTrigger_ImmediateExtracts_RenderView1_000000.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/WaveletTimeTrigger_CoprocessingExtracts_RenderView1_000000.png}"
)

set(simple_scripts
  ValidateChangingInput.py
  CatalystScriptCustomCallbackVerifier.py
  package_test
  package_test_zip.zip)

if (TARGET ParaView::RemotingLive)
  list(APPEND simple_scripts
      LiveOnly.py)
endif()

if (NOT PARAVIEW_SERIAL_TESTS_USE_MPIEXEC)
  # no need to add these non-MPI tests if serial tests are going to use MPI
  # anyways. The have another `paraview_add_test_mpi` invocation right after this
  # to handle it.
  paraview_add_test(
    PREFIX "Catalyst::WaveletMiniApp"
    ARGS   "$<TARGET_FILE:ParaView::pvbatch>" -dr -sym -m paraview.demos.wavelet_miniapp -t 20 --size 21 -s
    TEST_SCRIPTS ${simple_scripts})
endif()

paraview_add_test_mpi(
  PREFIX "Catalyst::MPI::WaveletMiniApp"
  ARGS   "$<TARGET_FILE:ParaView::pvbatch>" -dr -sym -m paraview.demos.wavelet_miniapp -t 20 --size 21 -s
  TEST_SCRIPTS ${simple_scripts})

foreach (tname IN LISTS scripts)
  if (NOT PARAVIEW_SERIAL_TESTS_USE_MPIEXEC)
    set_tests_properties("Catalyst::WaveletMiniApp.${tname}"
      PROPERTIES
        PASS_REGULAR_EXPRESSION "All ok")
  endif()
  if (PARAVIEW_USE_MPI)
    set_tests_properties("Catalyst::MPI::WaveletMiniApp.${tname}"
      PROPERTIES
        PASS_REGULAR_EXPRESSION "All ok")
  endif()
endforeach()

if (TARGET ParaView::RemotingViews)
  #--------------------------------------------------------------------
  # add test to validate multiple analysis pipelines can be used.
  add_test(NAME Catalyst::WaveletMiniApp::MultiplePipelines::Prepare
           COMMAND "${CMAKE_COMMAND}" -E remove_directory "${CMAKE_BINARY_DIR}/Testing/Temporary/MultiplePipelines")
  set_tests_properties(
    "Catalyst::WaveletMiniApp::MultiplePipelines::Prepare"
    PROPERTIES
      FIXTURES_SETUP "Catalyst::WaveletMiniApp::MultiplePipelines::Prepare")
  paraview_add_test(
      PREFIX "Catalyst::WaveletMiniApp::MultiplePipelines::pipeline1"
      ENVIRONMENT
        PARAVIEW_OVERRIDE_EXTRACTS_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/Testing/Temporary/MultiplePipelines/
      ARGS   "$<TARGET_FILE:ParaView::pvbatch>"
             -sym
             -dr
             -m paraview.demos.wavelet_miniapp -t 10 --size 21
             --channel Wavelet1
             -s "${CMAKE_CURRENT_SOURCE_DIR}/pipeline1.py"
             -s
      TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline2.py")
  set_tests_properties(
    "Catalyst::WaveletMiniApp::MultiplePipelines::pipeline1.pipeline2"
    PROPERTIES
      FIXTURES_REQUIRED "Catalyst::WaveletMiniApp::MultiplePipelines::Prepare")

  paraview_add_test(
      PREFIX "Catalyst::WaveletMiniApp::MultiplePipelines::Validate::pipeline1"
      ARGS   "$<TARGET_FILE:ParaView::pvpython>"
             -dr
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
endif()

#---------------------------------------------------------------------
# add tests for filedriver_miniapp
set(filedriver_tests
   ValidateChangingTime.py)

if (NOT PARAVIEW_SERIAL_TESTS_USE_MPIEXEC)
  # if PARAVIEW_SERIAL_TESTS_USE_MPIEXEC, the paraview_add_test_mpi
  # call and this will be duplicates, so no need to do these here.
  paraview_add_test(
    PREFIX "Catalyst::FileDriverMiniApp::Wavelet"
    ARGS  "$<TARGET_FILE:ParaView::pvbatch>" -dr -sym
          -m paraview.demos.filedriver_miniapp
          -g "${paraview_test_data_directory_output}/Testing/Data/Wavelet/*.pvti"
          -s
    TEST_SCRIPTS ${filedriver_tests})

  paraview_add_test(
    PREFIX "Catalyst::FileDriverMiniApp::Can"
    ARGS  "$<TARGET_FILE:ParaView::pvbatch>" -dr -sym
          -m paraview.demos.filedriver_miniapp
          -g "${paraview_test_data_directory_output}/Testing/Data/can_vtm/*.vtm"
          -s
    TEST_SCRIPTS ${filedriver_tests})
endif()

paraview_add_test_mpi(
  PREFIX "Catalyst::MPI::FileDriverMiniApp::Wavelet"
  ARGS  "$<TARGET_FILE:ParaView::pvbatch>" -dr -sym
        -m paraview.demos.filedriver_miniapp
        -g "${paraview_test_data_directory_output}/Testing/Data/Wavelet/*.pvti"
        -s
  TEST_SCRIPTS ${filedriver_tests})

paraview_add_test_mpi(
  PREFIX "Catalyst::MPI::FileDriverMiniApp::Can"
  ARGS  "$<TARGET_FILE:ParaView::pvbatch>" -dr -sym
        -m paraview.demos.filedriver_miniapp
        -g "${paraview_test_data_directory_output}/Testing/Data/can_vtm/*.vtm"
        -s
  TEST_SCRIPTS ${filedriver_tests})
unset(filedriver_tests)

#---------------------------------------------------------------------
# Let's add some full workflow tests.
# These workflow tests have several steps and require special care when
# creation. The following describes the steps involved.
#
# 1. Create the test XML: record a ParaView UI/XML test as usual with actions to
#    add extractors (which are necessary).
#
#    One of the steps in the test must be generation of extracts using
#    "General Extracts" action. During test playback the
#    output directory is set to `$PARAVIEW_TEST_ROOT/ImmediateExtracts` using
#    PARAVIEW_OVERRIDE_EXTRACTS_OUTPUT_DIRECTORY environment variable.
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
if (TARGET ParaView::paraview OR paraview_will_be_built)
  include(FindPythonModules)
  find_python_module(pandas pandas_found)

  set(catalyst_tests
    WaveletHistogramExtracts.xml
    WaveletTimeTrigger.xml)

  if (pandas_found)
    list(APPEND catalyst_tests
      WaveletPhiThetaExtractsWithCinema.xml,CINEMA,
      WaveletVolumeRenderWithCinema.xml,CINEMA)
  else ()
    message(WARNING "'pandas' is missing. Skipping Cinema tests")
  endif()

  set(catalyst_options
    CINEMA
    )

  _vtk_test_parse_args("${catalyst_options}" "xml" ${catalyst_tests})
  _vtk_test_set_options("${catalyst_options}" "" ${options})

  foreach (tname IN LISTS names)
    _vtk_test_set_options("${catalyst_options}" "local_" ${_${tname}_options})
    _vtk_test_parse_name("${tname}" "xml")

    set(ttemp_dir "${CMAKE_BINARY_DIR}/Testing/Temporary/${test_name}")

    #----------------------------------------------------------------------
    # Fixture setup
    # remove ttemp_dir as first step
    add_test(NAME Catalyst::Workflow::RemoveOld.${test_name}
             COMMAND "${CMAKE_COMMAND}" -E remove_directory "${ttemp_dir}")

    # create the ttemp_dir
    add_test(NAME Catalyst::Workflow::Prepare.${test_name}
             COMMAND "${CMAKE_COMMAND}" -E make_directory "${ttemp_dir}")

    set_tests_properties(
        "Catalyst::Workflow::Prepare.${test_name}"
      PROPERTIES
        FIXTURES_SETUP "CatalystWorkflowSetup.${test_name}"
        FIXTURES_REQUIRED "CatalystWorkflowPreSetup.${test_name}")

    set_tests_properties(
      "Catalyst::Workflow::RemoveOld.${test_name}"
      PROPERTIES
        FIXTURES_SETUP "CatalystWorkflowPreSetup.${test_name}")
    #----------------------------------------------------------------------

    # 1. run GUI to generate the pvsm state file and validate "export now"
    paraview_add_client_tests(
      PREFIX "Catalyst::Workflow::ParaView"
      TEST_DIRECTORY "${ttemp_dir}"
      TEST_SCRIPTS "${test_file}")

    set_tests_properties("Catalyst::Workflow::ParaView.${test_name}"
      PROPERTIES
        FIXTURES_REQUIRED "CatalystWorkflowSetup.${test_name}"
        ENVIRONMENT
            PARAVIEW_OVERRIDE_EXTRACTS_OUTPUT_DIRECTORY=${ttemp_dir}/ImmediateExtracts
        )

    # 1.1 validate 'ImmediateExtracts'
    paraview_add_test(
      PREFIX "Catalyst::Workflow::ParaView::Validate"
      ARGS "$<TARGET_FILE:ParaView::pvpython>"
           -dr
           -m paraview.tests.validate_extracts
           --name "${test_name}"
           --root  "${ttemp_dir}"
           --subdir "ImmediateExtracts"
           --baseline-dir "${paraview_test_data_directory_output}/Clients/PythonCatalyst/Testing/Data/Baseline"
           --temp-dir "${CMAKE_BINARY_DIR}/Testing/Temporary"
           --json
      TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/${test_name}.json")

    set_tests_properties(
      "Catalyst::Workflow::ParaView::Validate.${test_name}"
      PROPERTIES DEPENDS "Catalyst::Workflow::ParaView.${test_name}")

    # 2. add test to generate Catalyst Python state. We'll just use pvpython for
    #    this.
    set(cinema_args)
    if (local_CINEMA)
        set(cinema_args "--cinema")
    endif ()

    paraview_add_test(
      PREFIX "Catalyst::Workflow::SaveCatalystState"
      ARGS "$<TARGET_FILE:ParaView::pvpython>"
            -dr
            -m paraview.demos.export_catalyst_state
            --output "${ttemp_dir}/${test_name}.py"
            --extracts-dir "${ttemp_dir}/CoprocessingExtracts"
            ${cinema_args}
            --pvsm
      TEST_SCRIPTS ${ttemp_dir}/${test_name}.pvsm)

    set_tests_properties(
      "Catalyst::Workflow::SaveCatalystState.${test_name}"
      PROPERTIES DEPENDS "Catalyst::Workflow::ParaView.${test_name}")

    # 3. add test to load catalyst script in pvbatch.
    paraview_add_test_mpi_optional(
      PREFIX "Catalyst::Workflow::BatchExecute"
      ENVIRONMENT
        PARAVIEW_OVERRIDE_EXTRACTS_OUTPUT_DIRECTORY=${ttemp_dir}/BatchExtracts
      ARGS "$<TARGET_FILE:ParaView::pvbatch>"
           -dr
      TEST_SCRIPTS ${ttemp_dir}/${test_name}.py)

    set_tests_properties(
      "Catalyst::Workflow::BatchExecute.${test_name}"
      PROPERTIES DEPENDS
        "Catalyst::Workflow::SaveCatalystState.${test_name}")

    # 3.1 validate 'BatchExtracts'
    set(json_infix )
    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${test_name}.MPI.json AND PARAVIEW_USE_MPI)
      # MPI runs sometimes have different files generated. Handle that.
      set(json_infix ".MPI")
    endif()

    paraview_add_test(
      PREFIX "Catalyst::Workflow::BatchExecute::Validate"
      ARGS "$<TARGET_FILE:ParaView::pvpython>"
           -dr
           -m paraview.tests.validate_extracts
           --name "${test_name}"
           --root  "${ttemp_dir}"
           --subdir "BatchExtracts"
           --baseline-dir "${paraview_test_data_directory_output}/Clients/PythonCatalyst/Testing/Data/Baseline"
           --temp-dir "${CMAKE_BINARY_DIR}/Testing/Temporary"
           --json
      TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/${test_name}${json_infix}.json")

    set_tests_properties(
      "Catalyst::Workflow::BatchExecute::Validate.${test_name}"
      PROPERTIES DEPENDS "Catalyst::Workflow::BatchExecute.${test_name}")

    # 4. run with WaveletMiniApp
    paraview_add_test_mpi_optional(
      PREFIX "Catalyst::Workflow::WaveletMiniApp"
      ARGS "$<TARGET_FILE:ParaView::pvbatch>"
            -dr
            -sym -m paraview.demos.wavelet_miniapp -t 20 --size 21
            --channel Wavelet1
            -s
      TEST_SCRIPTS ${ttemp_dir}/${test_name}.py)

    set_tests_properties(
      "Catalyst::Workflow::WaveletMiniApp.${test_name}"
      PROPERTIES DEPENDS
        "Catalyst::Workflow::SaveCatalystState.${test_name}")

    # 4.1 validate 'CoprocessingExtracts'
    paraview_add_test(
      PREFIX "Catalyst::Workflow::WaveletMiniApp::Validate"
      ARGS "$<TARGET_FILE:ParaView::pvpython>"
           -dr
           -m paraview.tests.validate_extracts
           --name "${test_name}"
           --root  "${ttemp_dir}"
           --subdir "CoprocessingExtracts"
           --baseline-dir "${paraview_test_data_directory_output}/Clients/PythonCatalyst/Testing/Data/Baseline"
           --temp-dir "${CMAKE_BINARY_DIR}/Testing/Temporary"
           --json
      TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/${test_name}${json_infix}.json")

    set_tests_properties(
      "Catalyst::Workflow::WaveletMiniApp::Validate.${test_name}"
      PROPERTIES DEPENDS "Catalyst::Workflow::WaveletMiniApp.${test_name}")
  endforeach()
else()
  message(STATUS "ParaView client not built. Skipping Catalyst workflow tests.")
endif()
