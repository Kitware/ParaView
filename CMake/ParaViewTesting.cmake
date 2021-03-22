function (paraview_add_test_python)
  set(_vtk_testing_python_exe "$<TARGET_FILE:ParaView::pvpython>")
  set(_vtk_test_python_args -dr ${paraview_python_args})
  vtk_add_test_python(${ARGN})
endfunction ()

function (paraview_add_test_pvbatch)
  set(_vtk_testing_python_exe "$<TARGET_FILE:ParaView::pvbatch>")
  set(_vtk_test_python_args -dr ${paraview_pvbatch_args})
  set(vtk_test_prefix "Batch-${vtk_test_prefix}")
  vtk_add_test_python(${ARGN})
endfunction ()

function (paraview_add_test_pvbatch_mpi)
  set(_vtk_testing_python_exe "$<TARGET_FILE:ParaView::pvbatch>")
  set(_vtk_test_python_args -dr ${paraview_pvbatch_args})
  set(vtk_test_prefix "Batch-${vtk_test_prefix}")
  vtk_add_test_python_mpi(${ARGN})
endfunction ()

function(paraview_add_test_driven)
  if (NOT (TARGET pvserver AND TARGET pvpython))
    return()
  endif ()
  set(_vtk_testing_python_exe "$<TARGET_FILE:ParaView::smTestDriver>")
  set(_vtk_test_python_args
    --server $<TARGET_FILE:ParaView::pvserver>
    --client $<TARGET_FILE:ParaView::pvpython> -dr)
  vtk_add_test_python(${ARGN})
endfunction ()

function (_paraview_add_tests function)
  cmake_parse_arguments(_paraview_add_tests
    "FORCE_SERIAL;FORCE_LOCK"
    "LOAD_PLUGIN;PLUGIN_PATH;CLIENT;TEST_DIRECTORY;TEST_DATA_TARGET;PREFIX;SUFFIX;_ENABLE_SUFFIX;_DISABLE_SUFFIX;BASELINE_DIR;DATA_DIRECTORY;NUMPROCS"
    "_COMMAND_PATTERN;LOAD_PLUGINS;PLUGIN_PATHS;TEST_SCRIPTS;TEST_NAME;ENVIRONMENT;ARGS;CLIENT_ARGS"
    ${ARGN})

  if (_paraview_add_tests_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for ${function}: "
      "${_paraview_add_tests_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _paraview_add_tests__COMMAND_PATTERN)
    message(FATAL_ERROR
      "The `_COMMAND_PATTERN` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_add_tests_CLIENT)
    if ("__paraview_client__" IN_LIST _paraview_add_tests__COMMAND_PATTERN # do we care?
        AND NOT TARGET ParaView::paraview # For external testing.
        AND NOT paraview_will_be_built) # For use within ParaView itself.
      return ()
    endif ()

    set(_paraview_add_tests_CLIENT
      "$<TARGET_FILE:ParaView::paraview>")
  endif ()

  if (NOT DEFINED _paraview_add_tests_PREFIX)
    set(_paraview_add_tests_PREFIX "pv")
  endif ()

  if (NOT DEFINED _paraview_add_tests_TEST_DIRECTORY)
    set(_paraview_add_tests_TEST_DIRECTORY
      "${CMAKE_BINARY_DIR}/Testing/Temporary")
  endif ()

  if (NOT DEFINED _paraview_add_tests_DATA_DIRECTORY AND DEFINED _paraview_add_tests_default_data_directory)
    set(_paraview_add_tests_DATA_DIRECTORY
      "${_paraview_add_tests_default_data_directory}")
  endif ()

  if (DEFINED _paraview_add_tests_TEST_SCRIPTS AND DEFINED _paraview_add_tests_TEST_NAME)
    message(FATAL_ERROR
      "Only one of `TEST_NAME` or `TEST_SCRIPTS` must be specified.")
  elseif (NOT DEFINED _paraview_add_tests_TEST_SCRIPTS AND NOT DEFINED _paraview_add_tests_TEST_NAME)
    message(FATAL_ERROR
      "Either `TEST_SCRIPTS` or `TEST_NAME` must be specified.")
  endif()

  set(_paraview_add_tests_args
    ${_paraview_add_tests_ARGS})

  if (DEFINED _paraview_add_tests_PLUGIN_PATH)
    if (DEFINED _paraview_add_tests_PLUGIN_PATHS)
      message(FATAL_ERROR
        "The `PLUGIN_PATH` argument is incompatible "
        "with `PLUGIN_PATHS`.")
    endif ()
    list(APPEND _paraview_add_tests_args
      "--test-plugin-path=${_paraview_add_tests_PLUGIN_PATH}")
  endif ()

  if (DEFINED _paraview_add_tests_PLUGIN_PATHS)
    string(REPLACE ";" "," _plugin_paths "${_paraview_add_tests_PLUGIN_PATHS}")
    list(APPEND _paraview_add_tests_args
      "--test-plugin-paths=${_plugin_paths}")
  endif ()

  if (DEFINED _paraview_add_tests_LOAD_PLUGIN)
    if (DEFINED _paraview_add_tests_LOAD_PLUGINS)
      message(FATAL_ERROR
        "The `LOAD_PLUGIN` argument is incompatible "
        "with `LOAD_PLUGINS`.")
    endif ()
    list(APPEND _paraview_add_tests_args
      "--test-plugin=${_paraview_add_tests_LOAD_PLUGIN}")
  endif ()

  if (DEFINED _paraview_add_tests_LOAD_PLUGINS)
    string(REPLACE ";" "," _load_plugins "${_paraview_add_tests_LOAD_PLUGINS}")
    list(APPEND _paraview_add_tests_args
      "--test-plugins=${_load_plugins}")
  endif ()

  string(REPLACE "__paraview_args__" "${_paraview_add_tests_args}"
    _paraview_add_tests__COMMAND_PATTERN
    "${_paraview_add_tests__COMMAND_PATTERN}")
  string(REPLACE "__paraview_client__" "${_paraview_add_tests_CLIENT}"
    _paraview_add_tests__COMMAND_PATTERN
    "${_paraview_add_tests__COMMAND_PATTERN}")

  foreach (_paraview_add_tests_script IN LISTS _paraview_add_tests_TEST_SCRIPTS _paraview_add_tests_TEST_NAME)
    if (DEFINED _paraview_add_tests_TEST_NAME)
      set(_paraview_add_tests_name "${_paraview_add_tests_script}")
      set(_paraview_add_tests_name_base "${_paraview_add_tests_name}")
      set(_paraview_add_tests_script)
    else()
      if (NOT IS_ABSOLUTE "${_paraview_add_tests_script}")
        set(_paraview_add_tests_script
          "${CMAKE_CURRENT_SOURCE_DIR}/${_paraview_add_tests_script}")
      endif ()
      get_filename_component(_paraview_add_tests_name "${_paraview_add_tests_script}" NAME_WE)
      set(_paraview_add_tests_name_base "${_paraview_add_tests_name}")
    endif()

    string(APPEND _paraview_add_tests_name "${_paraview_add_tests_SUFFIX}")

    if (DEFINED _paraview_add_tests__DISABLE_SUFFIX AND ${_paraview_add_tests_name}${_paraview_add_tests__DISABLE_SUFFIX})
      continue ()
    endif ()

    if (DEFINED _paraview_add_tests__ENABLE_SUFFIX AND NOT ${_paraview_add_tests_name}${_paraview_add_tests__ENABLE_SUFFIX})
      continue ()
    endif ()

    if (NOT DEFINED _paraview_add_tests_TEST_DATA_TARGET)
      if (DEFINED _paraview_add_tests_default_test_data_target)
        set(_paraview_add_tests_TEST_DATA_TARGET
          "${_paraview_add_tests_default_test_data_target}")
      else()
        if (NOT DEFINED "${_paraview_add_tests_name}_USES_DIRECT_DATA")
          message(FATAL_ERROR "The `TEST_DATA_TARGET` argument is required.")
        endif()
      endif ()
    endif ()

    # Build arguments to pass to the clients.
    set(_paraview_add_tests_client_args
      "--test-directory=${_paraview_add_tests_TEST_DIRECTORY}"
      ${_paraview_add_tests_CLIENT_ARGS})
    if (DEFINED _paraview_add_tests_BASELINE_DIR)
      if (DEFINED "${_paraview_add_tests_name}_BASELINE")
        if (DEFINED "${_paraview_add_tests_name}_USES_DIRECT_DATA")
          list(APPEND _paraview_add_tests_client_args
            "--test-baseline=${_paraview_add_tests_BASELINE_DIR}/${${_paraview_add_tests_name_base}_BASELINE}")
        else()
          list(APPEND _paraview_add_tests_client_args
            "--test-baseline=DATA{${_paraview_add_tests_BASELINE_DIR}/${${_paraview_add_tests_name_base}_BASELINE}}")
        endif ()
      else ()
        if (DEFINED "${_paraview_add_tests_name}_USES_DIRECT_DATA")
          list(APPEND _paraview_add_tests_client_args
            "--test-baseline=${_paraview_add_tests_BASELINE_DIR}/${_paraview_add_tests_name_base}.png")
        else()
          list(APPEND _paraview_add_tests_client_args
            "--test-baseline=DATA{${_paraview_add_tests_BASELINE_DIR}/${_paraview_add_tests_name_base}.png}")
        endif ()
      endif ()
      if (NOT DEFINED "${_paraview_add_tests_name}_USES_DIRECT_DATA")
        ExternalData_Expand_Arguments("${_paraview_add_tests_TEST_DATA_TARGET}" _
          "DATA{${_paraview_add_tests_BASELINE_DIR}/,REGEX:${_paraview_add_tests_name_base}(-.*)?(_[0-9]+)?.png}")
      endif()
    endif ()
    if (DEFINED "${_paraview_add_tests_name}_THRESHOLD")
      list(APPEND _paraview_add_tests_client_args
        "--test-threshold=${${_paraview_add_tests_name}_THRESHOLD}")
    endif ()
    if (DEFINED _paraview_add_tests_DATA_DIRECTORY)
      list(APPEND _paraview_add_tests_client_args
        "--data-directory=${_paraview_add_tests_DATA_DIRECTORY}")
    endif ()

    if (DEFINED _paraview_add_tests_TEST_NAME)
      string(REPLACE "__paraview_script__" ""
        _paraview_add_tests_script_args
        "${_paraview_add_tests__COMMAND_PATTERN}")
      string(REPLACE "__paraview_scriptpath__" ""
        _paraview_add_tests_script_args
        "${_paraview_add_tests_script_args}")
    else()
      string(REPLACE "__paraview_script__" "--test-script=${_paraview_add_tests_script}"
        _paraview_add_tests_script_args
        "${_paraview_add_tests__COMMAND_PATTERN}")
      string(REPLACE "__paraview_scriptpath__" "${_paraview_add_tests_script}"
        _paraview_add_tests_script_args
        "${_paraview_add_tests_script_args}")
    endif()

    string(REPLACE "__paraview_client_args__" "${_paraview_add_tests_client_args}"
      _paraview_add_tests_script_args
      "${_paraview_add_tests_script_args}")
    string(REPLACE "__paraview_test_name__" "${_paraview_add_tests_name_base}"
      _paraview_add_tests_script_args
      "${_paraview_add_tests_script_args}")

    set(testArgs
        NAME    "${_paraview_add_tests_PREFIX}.${_paraview_add_tests_name}"
        COMMAND ParaView::smTestDriver
                --enable-bt
                ${_paraview_add_tests_script_args})
    if (DEFINED "${_paraview_add_tests_name}_USES_DIRECT_DATA")
      add_test(${testArgs})
    else()
      ExternalData_add_test("${_paraview_add_tests_TEST_DATA_TARGET}" ${testArgs})
    endif()

    set_property(TEST "${_paraview_add_tests_PREFIX}.${_paraview_add_tests_name}"
      PROPERTY
        LABELS ParaView)
    set_property(TEST "${_paraview_add_tests_PREFIX}.${_paraview_add_tests_name}"
      PROPERTY
        ENVIRONMENT "${_paraview_add_tests_ENVIRONMENT}")
    if (DEFINED _paraview_add_tests_NUMPROCS)
      set_property(TEST "${_paraview_add_tests_PREFIX}.${_paraview_add_tests_name}"
        PROPERTY
          PROCESSORS "${_paraview_add_tests_NUMPROCS}")
    endif ()
    if (${_paraview_add_tests_name}_FORCE_SERIAL OR _paraview_add_tests_FORCE_SERIAL)
      set_property(TEST "${_paraview_add_tests_PREFIX}.${_paraview_add_tests_name}"
        PROPERTY
          RUN_SERIAL ON)
    elseif (NOT DEFINED _paraview_add_tests_TEST_NAME AND EXISTS "${_paraview_add_tests_script}")
      # if the XML test contains PARAVIEW_TEST_ROOT we assume that we may be writing
      # to that file and reading it back in so we add a resource lock on the XML
      # file so that the pv.X, pvcx.X and pvcrs.X tests don't run simultaneously.
      # we only need to do this if the test isn't forced to be serial already.
      if (NOT IS_DIRECTORY "${_paraview_add_tests_script}" AND NOT ${_paraview_add_tests_name}_FORCE_LOCK)
        file(STRINGS "${_paraview_add_tests_script}" _paraview_add_tests_paraview_test_root REGEX PARAVIEW_TEST_ROOT)
      endif ()
      if (${_paraview_add_tests_name}_FORCE_LOCK OR _paraview_add_tests_paraview_test_root)
        set_property(TEST "${_paraview_add_tests_PREFIX}.${_paraview_add_tests_name}"
          PROPERTY
            RESOURCE_LOCK "${_paraview_add_tests_script}")
      endif ()
    endif ()
  endforeach ()
endfunction ()

function(_get_prefix varname default)
  cmake_parse_arguments(_get_prefix
    ""
    "PREFIX"
    ""
    ${ARGN})
  if (_get_prefix_PREFIX)
    set(${varname} "${_get_prefix_PREFIX}" PARENT_SCOPE)
  else()
    set(${varname} "${default}" PARENT_SCOPE)
  endif()
endfunction()

function (paraview_add_client_tests)
  _get_prefix(chosen_prefix "pv" ${ARGN})
  _paraview_add_tests("paraview_add_client_tests"
    PREFIX "${chosen_prefix}"
    _DISABLE_SUFFIX "_DISABLE_C"
    _COMMAND_PATTERN
      --client __paraview_client__
        --enable-bt
        __paraview_args__
        __paraview_script__
        __paraview_client_args__
        -dr
        --exit
    ${ARGN})
endfunction ()

function (paraview_add_client_server_tests)
  _get_prefix(chosen_prefix "pvcs" ${ARGN})
  _paraview_add_tests("paraview_add_client_server_tests"
    PREFIX "${chosen_prefix}"
    _DISABLE_SUFFIX "_DISABLE_CS"
    _COMMAND_PATTERN
      --server "$<TARGET_FILE:ParaView::pvserver>"
        --enable-bt
        __paraview_args__
      --client __paraview_client__
        --enable-bt
        __paraview_args__
        __paraview_script__
        __paraview_client_args__
        -dr
        --exit
    ${ARGN})
endfunction ()

function (paraview_add_client_server_render_tests)
  _get_prefix(chosen_prefix "pvcrs" ${ARGN})
  _paraview_add_tests("paraview_add_client_server_render_tests"
    PREFIX "${chosen_prefix}"
    _DISABLE_SUFFIX "_DISABLE_CRS"
    _COMMAND_PATTERN
      --data-server "$<TARGET_FILE:ParaView::pvdataserver>"
        --enable-bt
        __paraview_args__
      --render-server "$<TARGET_FILE:ParaView::pvrenderserver>"
        --enable-bt
        __paraview_args__
      --client __paraview_client__
        --enable-bt
        __paraview_args__
        __paraview_script__
        __paraview_client_args__
        -dr
        --exit
    ${ARGN})
endfunction ()

function (paraview_add_multi_client_tests)
  _get_prefix(chosen_prefix "pvcs-multi-clients" ${ARGN})
  _paraview_add_tests("paraview_add_multi_client_tests"
    PREFIX "${chosen_prefix}"
    _ENABLE_SUFFIX "_ENABLE_MULTI_CLIENT"
    FORCE_SERIAL
    _COMMAND_PATTERN
      --test-multi-clients
      --server "$<TARGET_FILE:ParaView::pvserver>"
        --enable-bt
        __paraview_args__
      --client __paraview_client__
        --enable-bt
        __paraview_args__
        __paraview_script__
        __paraview_client_args__
        --test-master
        -dr
        --exit
      --client __paraview_client__
        --enable-bt
        __paraview_args__
        __paraview_client_args__
        --test-slave
        -dr
        --exit
    ${ARGN})
endfunction ()

function (paraview_add_multi_server_tests count)
  _get_prefix(chosen_prefix "pvcs-multi-servers" ${ARGN})
  _paraview_add_tests("paraview_add_multi_server_tests"
    PREFIX "${chosen_prefix}"
    SUFFIX "-${count}"
    _COMMAND_PATTERN
      --test-multi-servers "${count}"
      --server "$<TARGET_FILE:ParaView::pvserver>"
        --enable-bt
      --client __paraview_client__
        --enable-bt
        __paraview_args__
        __paraview_script__
        __paraview_client_args__
        -dr
        --exit
    ${ARGN})
endfunction ()

function (paraview_add_tile_display_tests width height)
  math(EXPR _paraview_add_tile_display_cpu_count "${width} * ${height}")

  if (_paraview_add_tile_display_cpu_count GREATER 1 AND NOT PARAVIEW_USE_MPI)
    # we can run 1x1 tile display tests on non MPI builds.
    return ()
  endif ()

  _get_prefix(chosen_prefix "pvcs-tile-display" ${ARGN})
  _paraview_add_tests("paraview_add_tile_display_tests"
    PREFIX "${chosen_prefix}"
    SUFFIX "-${width}x${height}"
    ENVIRONMENT
      PV_SHARED_WINDOW_SIZE=800x600
      SMTESTDRIVER_MPI_NUMPROCS=${_paraview_add_tile_display_cpu_count}
    NUMPROCS "${_paraview_add_tile_display_cpu_count}"
    _COMMAND_PATTERN
      --server "$<TARGET_FILE:ParaView::pvserver>"
        --enable-bt
        -tdx=${width}
        -tdy=${height}
        # using offscreen to avoid clobbering display (although should not be
        # necessary) when running tests in parallel.
        --force-offscreen-rendering
      --client __paraview_client__
        --enable-bt
        __paraview_args__
        __paraview_script__
        __paraview_client_args__
        -dr
        --exit
    ${ARGN})
endfunction ()

function (paraview_add_cave_tests num_ranks config)
  if (num_ranks GREATER 1 AND NOT PARAVIEW_USE_MPI)
    return ()
  endif ()

  get_filename_component(_config_name "${config}" NAME_WE)

  _get_prefix(chosen_prefix "pvcs-cave-${_config_name}" ${ARGN})
  _paraview_add_tests("paraview_add_cave_tests"
    PREFIX "${chosen_prefix}"
    SUFFIX "-${num_ranks}"
    ENVIRONMENT
      PV_SHARED_WINDOW_SIZE=400x300
      SMTESTDRIVER_MPI_NUMPROCS=${num_ranks}
    NUMPROCS "${num_ranks}"
    _COMMAND_PATTERN
      --server "$<TARGET_FILE:ParaView::pvserver>"
        --enable-bt
        # using offscreen to avoid clobbering display (although should not be
        # necessary) when running tests in parallel.
        --force-offscreen-rendering
        ${config}
      --client __paraview_client__
        --enable-bt
        __paraview_args__
        __paraview_script__
        __paraview_client_args__
        -dr
        --exit
    ${ARGN})
endfunction ()

# This is a catch-all function to run any custom command as the "client"
# using th smTestDriver. Note, the command must print "Process started" for
# smTestDriver to treat it as started otherwise the test will fail.
# The command to execute is passed as {ARGN} and is suffixed by each of the
# TEST_SCRIPTS provided, one at at time.
function (paraview_add_test)
  _get_prefix(chosen_prefix "paraview" ${ARGN})
  _paraview_add_tests("paraview_add_test"
    PREFIX "${chosen_prefix}"
    _COMMAND_PATTERN
      --client
      __paraview_args__
      __paraview_scriptpath__
    ${ARGN})
endfunction ()

# Same as `paraview_add_test` except makes smTestDriver run the command using
# mpi. If `PARAVIEW_USE_MPI` if not defined, this does not add any test.
function (paraview_add_test_mpi)
  if (PARAVIEW_USE_MPI)
    _get_prefix(chosen_prefix "paraview-mpi" ${ARGN})
    _paraview_add_tests("paraview_add_test_mpi"
      PREFIX "${chosen_prefix}"
      NUMPROCS 2 # See Utilities/TestDriver/CMakeLists.txt (PARAVIEW_MPI_MAX_NUMPROCS)
      _COMMAND_PATTERN
        --client-mpi
        __paraview_args__
        __paraview_scriptpath__
      ${ARGN})
  endif ()
endfunction ()


# if PARAVIEW_USE_MPI, calls paraview_add_test_mpi(), else calls
# paraview_add_test()
function (paraview_add_test_mpi_optional)
  if (PARAVIEW_USE_MPI)
    paraview_add_test_mpi(${ARGN})
  else()
    paraview_add_test(${ARGN})
  endif()
endfunction()
