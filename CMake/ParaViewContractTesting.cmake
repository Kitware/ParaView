#=========================================================================
#
#  This software is distributed WITHOUT ANY WARRANTY; without even
#  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#  PURPOSE.  See the above copyright notice for more information.
#
#=========================================================================

# paraview_contract_test(
#   TEST_FILE_URL <test_file>
#   [EXTRA_CMAKE_ARGUMENTS <cmake_arg>...]
# )
#
# add a test that builds and tests a suite of contract tests  as described in the test
#  file.
# note: This is derived from https://gitlab.kitware.com/cmb/smtk/-/blob/master/CMake/SMTKPluginTestingMacros.cmake
function (paraview_contract_test)
   cmake_parse_arguments(PARSE_ARGV 0 _paraview_contract_test
    ""
    "TEST_FILE_URL"
    "EXTRA_CMAKE_ARGUMENTS")

  if (_paraview_contract_test_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unrecognized arguments for paraview_contract_test: "
      "${_paraview_contract_test_UNPARSED_ARGUMENTS}.")
  endif ()

  if (NOT DEFINED _paraview_contract_test_TEST_FILE_URL)
    message(FATAL_ERROR
      "The `TEST_FILE_URL` argument is required.")
  endif ()
  set(test_file_url "${_paraview_contract_test_TEST_FILE_URL}")

  # Create a testing directory for the contract test based off of its hashed
  # file name.
  string(MD5 hashed_test_dir "${test_file_url}")
  string(SUBSTRING "${hashed_test_dir}" 0 8 hashed_test_dir)
  set(test_dir "${CMAKE_BINARY_DIR}/ContractTests/${hashed_test_dir}")

  # Set up a source directory for the contract test
  set(src_dir "${test_dir}/src")
  file(MAKE_DIRECTORY "${src_dir}")

  # Set up a build directory for the contract test
  set(build_dir "${test_dir}/build")
  file(MAKE_DIRECTORY "${build_dir}")

  # Download the contract file into the source directory.
  file(DOWNLOAD "${test_file_url}" "${src_dir}/CMakeLists.txt")

  # Check result for success
  if (NOT EXISTS "${src_dir}/CMakeLists.txt")
    message(WARNING "Cannot download contract test file <${test_file_url}>.")
    return ()
  endif ()

  # Derive a test name from the contract file name.
  get_filename_component(test_name ${test_file_url} NAME_WE)
  set(test_name "contract-${test_name}")

  set(ctest_extra_args)
  if (CMAKE_MAKE_PROGRAM)
    list(APPEND ctest_extra_args
      --build-makeprogram "${CMAKE_MAKE_PROGRAM}")
  endif ()

  if (CMAKE_GENERATOR_PLATFORM)
    list(APPEND ctest_extra_args
      --build-generator-platform "${CMAKE_GENERATOR_PLATFORM}")
  endif ()

  if (CMAKE_GENERATOR_TOOLSET)
    list(APPEND ctest_extra_args
      --build-generator-toolset "${CMAKE_GENERATOR_TOOLSET}")
  endif ()

  set(cmake_extra_args)
  if(_paraview_contract_test_EXTRA_CMAKE_ARGUMENTS)
    list(APPEND cmake_extra_args
      ${_paraview_contract_test_EXTRA_CMAKE_ARGUMENTS})
  endif()

  # Add a test that builds and tests the plugin, but does not install it.
  add_test(NAME "${test_name}"
    COMMAND "${CMAKE_CTEST_COMMAND}"
      --build-and-test "${src_dir}" "${build_dir}"
      --build-generator "${CMAKE_GENERATOR}"
      ${ctest_extra_args}
      --build-options
        -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
        -DBUILD_TESTING=ON
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        "-Dcatalyst_DIR=${catalyst_DIR}"
        "-DParaViewCatalyst_DIR=${ParaView_BINARY_DIR}/${paraview_catalyst_directory}"
        ${cmake_extra_args}
  )

  # Contract tests require compiling code, which can take up a lot of memory.
  # Running them serially may prevent memory-related issues (c++: fatal error:
  # Killed signal terminated program cc1plus) that sporadically appear on our
  # dashboards.
  #
  # Ideally, `ctest --build-and-test` would support setting load-level
  # parallelism so that tests can cooperatively work, but that still leaves
  # testing to take up a lot of resources. Best to leave as serial for now.
  set_property(TEST "${test_name}"
    PROPERTY
      RUN_SERIAL TRUE)

  set_property(TEST "${test_name}" APPEND
    PROPERTY
      ENVIRONMENT "CTEST_OUTPUT_ON_FAILURE=1")

  # The contract tests all create an ExternalProject, and it is that inner
  # example project that needs to find_package(catalyst), which in the case
  # of external Conduit, will require finding Conduit. Passing the Conduit
  # location via an environment variable makes it available to find_package.
  if (Conduit_DIR)
    set_property(TEST "${test_name}" APPEND
      PROPERTY
        ENVIRONMENT "Conduit_DIR=${Conduit_DIR}")
  endif ()

  # Any pipeline touching "info.catalyst_params" fails if conduit python can't
  # be imported. Until Catalyst includes the location of the Conduit python
  # modules in CATALYST_PYTHONPATH, take it from the VTK::conduit_python target
  # property. If that target exists, then ParaView/VTK and Catalyst depend on
  # the same Conduit installation, and the VTK target property path is the same
  # one that CATALYST_PYTHONPATH would contain.
  if (TARGET VTK::conduit_python)
    get_target_property(conduit_python_module_dir
      VTK::conduit_python VTK_CONDUIT_PYTHON_MODULE_DIR)
    if (conduit_python_module_dir)
      set_property(TEST "${test_name}" APPEND
        PROPERTY
          ENVIRONMENT_MODIFICATION
            "PYTHONPATH=path_list_prepend:${conduit_python_module_dir}")

      # Print a warning for older cmake versions
      if (CMAKE_VERSION VERSION_LESS "3.22")
        message(WARNING
          "CMake 3.22+ is required to reliably set up PYTHONPATH for contract "
          "tests. ${test_name} may fail to import the conduit module and fail "
          "as a result.")
      endif ()
    endif ()
  endif ()

  # The CI image has multiple libcatalyst with the same soname (one per MPI
  # flavor under /usr and /usr/lib64/<mpi> using Catalyst's mangled Conduit,
  # plus one per MPI flavor under /opt/catalyst-ext using the external Conduit
  # installs. Here we make sure that the example loads the one that matches the
  # ParaView build. Since the `catalyst::catalyst` target is not visible from
  # here, derive the directory from catalyst_DIR.
  if (catalyst_DIR AND UNIX AND NOT APPLE)
    get_filename_component(catalyst_library_dir "${catalyst_DIR}" DIRECTORY)
    get_filename_component(catalyst_library_dir "${catalyst_library_dir}" DIRECTORY)
    set_property(TEST "${test_name}" APPEND
      PROPERTY
        ENVIRONMENT_MODIFICATION
          "LD_LIBRARY_PATH=path_list_prepend:${catalyst_library_dir}")

    # Print a warning for older cmake versions
    if (CMAKE_VERSION VERSION_LESS "3.22")
      message(WARNING
        "CMake 3.22+ is required to reliably set up the environment for contract "
        "tests needing to load a specific Catalyst. ${test_name} may fail to load "
        "the correct catalyst and fail as a result.")
    endif ()
  endif ()

  set_tests_properties(${test_name} PROPERTIES LABELS "ParaViewContract")
endfunction ()
