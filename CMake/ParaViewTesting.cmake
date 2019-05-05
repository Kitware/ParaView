function (paraview_add_test_python)
  set(_vtk_testing_python_exe
    "$<TARGET_FILE:ParaView::pvpython>"
    -dr
    ${paraview_python_args})
  vtk_add_test_python(${ARGN})
endfunction ()

function (paraview_add_test_python_mpi)
  set(_vtk_testing_python_exe
    "$<TARGET_FILE:ParaView::pvpython>"
    -dr
    ${paraview_python_args})
  vtk_add_test_python_mpi(${ARGN})
endfunction ()

function (paraview_add_test_pvbatch)
  set(_vtk_testing_python_exe
    "$<TARGET_FILE:ParaView::pvbatch>"
    -dr
    ${paraview_pvbatch_args})
  set(vtk_test_prefix "Batch-${vtk_test_prefix}")
  vtk_add_test_python(${ARGN})
endfunction ()

function (paraview_add_test_pvbatch_mpi)
  set(_vtk_testing_python_exe
    "$<TARGET_FILE:ParaView::pvbatch>"
    -dr
    ${paraview_pvbatch_args})
  set(vtk_test_prefix "Batch-${vtk_test_prefix}")
  vtk_add_test_python_mpi(${ARGN})
endfunction ()

function(paraview_add_test_driven)
  if (NOT (TARGET pvserver AND TARGET pvpython))
    return()
  endif ()
  set(_vtk_testing_python_exe "$<TARGET_FILE:ParaView::smTestDriver>")
  list(APPEND VTK_PYTHON_ARGS
    --server $<TARGET_FILE:ParaView::pvserver>
    --client $<TARGET_FILE:ParaView::pvpython> -dr)
  vtk_add_test_python(${ARGN})
endfunction ()

function (_paraview_add_tests function)
  cmake_parse_arguments(_paraview_add_tests
    "FORCE_SERIAL;FORCE_LOCK"
    "LOAD_PLUGIN;PLUGIN_PATH;CLIENT;TEST_DIRECTORY;TEST_DATA_TARGET;PREFIX;SUFFIX;_ENABLE_SUFFIX;_DISABLE_SUFFIX;BASELINE_DIR;DATA_DIRECTORY"
    "_COMMAND_PATTERN;TEST_SCRIPTS;ENVIRONMENT;ARGS;CLIENT_ARGS"
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

  if (NOT DEFINED _paraview_add_tests_TEST_DATA_TARGET)
    if (DEFINED _paraview_add_tests_default_test_data_target)
      set(_paraview_add_tests_TEST_DATA_TARGET
        "${_paraview_add_tests_default_test_data_target}")
    else ()
      message(FATAL_ERROR
        "The `TEST_DATA_TARGET` argument is required.")
    endif ()
  endif ()

  if (NOT DEFINED _paraview_add_tests_CLIENT)
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

  set(_paraview_add_tests_args
    ${_paraview_add_tests_ARGS})

  if (DEFINED _paraview_add_tests_PLUGIN_PATH)
    list(APPEND _paraview_add_tests_args
      "--test-plugin-path=${_paraview_add_tests_PLUGIN_PATH}")
  endif ()

  if (DEFINED _paraview_add_tests_LOAD_PLUGIN)
    list(APPEND _paraview_add_tests_args
      "--test-plugin=${_paraview_add_tests_LOAD_PLUGIN}")
  endif ()

  string(REPLACE "__paraview_args__" "${_paraview_add_tests_args}"
    _paraview_add_tests__COMMAND_PATTERN
    "${_paraview_add_tests__COMMAND_PATTERN}")
  string(REPLACE "__paraview_client__" "${_paraview_add_tests_CLIENT}"
    _paraview_add_tests__COMMAND_PATTERN
    "${_paraview_add_tests__COMMAND_PATTERN}")

  foreach (_paraview_add_tests_script IN LISTS _paraview_add_tests_TEST_SCRIPTS)
    if (NOT IS_ABSOLUTE "${_paraview_add_tests_script}")
      set(_paraview_add_tests_script
        "${CMAKE_CURRENT_SOURCE_DIR}/${_paraview_add_tests_script}")
    endif ()
    get_filename_component(_paraview_add_tests_name "${_paraview_add_tests_script}" NAME_WE)
    set(_paraview_add_tests_name_base "${_paraview_add_tests_name}")
    string(APPEND _paraview_add_tests_name "${_paraview_add_tests_SUFFIX}")

    if (DEFINED _paraview_add_tests__DISABLE_SUFFIX AND ${_paraview_add_tests_name}${_paraview_add_tests__DISABLE_SUFFIX})
      continue ()
    endif ()

    if (DEFINED _paraview_add_tests__ENABLE_SUFFIX AND NOT ${_paraview_add_tests_name}${_paraview_add_tests__ENABLE_SUFFIX})
      continue ()
    endif ()

    # Build arguments to pass to the clients.
    set(_paraview_add_tests_client_args
      "--test-directory=${_paraview_add_tests_TEST_DIRECTORY}"
      ${_paraview_add_tests_CLIENT_ARGS})
    if (DEFINED _paraview_add_tests_BASELINE_DIR)
      if (DEFINED "${_paraview_add_tests_name}_BASELINE")
        list(APPEND _paraview_add_tests_client_args
          "--test-baseline=DATA{${_paraview_add_tests_BASELINE_DIR}/${${_paraview_add_tests_name_base}_BASELINE}}")
      else ()
        list(APPEND _paraview_add_tests_client_args
          "--test-baseline=DATA{${_paraview_add_tests_BASELINE_DIR}/${_paraview_add_tests_name_base}.png}")
      endif ()
      ExternalData_Expand_Arguments("${_paraview_add_tests_TEST_DATA_TARGET}" _
        "DATA{${_paraview_add_tests_BASELINE_DIR}/,REGEX:${_paraview_add_tests_name_base}(-.*)?(_[0-9]+)?.png}")
    endif ()
    if (DEFINED "${_paraview_add_tests_name}_THRESHOLD")
      list(APPEND _paraview_add_tests_client_args
        "--test-threshold=${${_paraview_add_tests_name}_THRESHOLD}")
    endif ()
    if (DEFINED _paraview_add_tests_DATA_DIRECTORY)
      list(APPEND _paraview_add_tests_client_args
        "--data-directory=${_paraview_add_tests_DATA_DIRECTORY}")
    endif ()

    string(REPLACE "__paraview_script__" "--test-script=${_paraview_add_tests_script}"
      _paraview_add_tests_script_args
      "${_paraview_add_tests__COMMAND_PATTERN}")
    string(REPLACE "__paraview_client_args__" "${_paraview_add_tests_client_args}"
      _paraview_add_tests_script_args
      "${_paraview_add_tests_script_args}")
    string(REPLACE "__paraview_test_name__" "${_paraview_add_tests_name_base}"
      _paraview_add_tests_script_args
      "${_paraview_add_tests_script_args}")

    ExternalData_add_test("${_paraview_add_tests_TEST_DATA_TARGET}"
      NAME    "${_paraview_add_tests_PREFIX}.${_paraview_add_tests_name}"
      COMMAND ParaView::smTestDriver
              --enable-bt
              ${_paraview_add_tests_script_args})
    set_property(TEST "${_paraview_add_tests_PREFIX}.${_paraview_add_tests_name}"
      PROPERTY
        LABELS ParaView)
    set_property(TEST "${_paraview_add_tests_PREFIX}.${_paraview_add_tests_name}"
      PROPERTY
        ENVIRONMENT "${_paraview_add_tests_ENVIRONMENT}")
    if (${_paraview_add_tests_name}_FORCE_SERIAL OR _paraview_add_tests_FORCE_SERIAL)
      set_property(TEST "${_paraview_add_tests_PREFIX}.${_paraview_add_tests_name}"
        PROPERTY
          RUN_SERIAL ON)
    else ()
      # if the XML test contains PARAVIEW_TEST_ROOT we assume that we may be writing
      # to that file and reading it back in so we add a resource lock on the XML
      # file so that the pv.X, pvcx.X and pvcrs.X tests don't run simultaneously.
      # we only need to do this if the test isn't forced to be serial already.
      if (NOT ${_paraview_add_tests_name}_FORCE_LOCK)
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

function (paraview_add_client_tests)
  _paraview_add_tests("paraview_add_client_tests"
    PREFIX "pv"
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
  _paraview_add_tests("paraview_add_client_server_tests"
    PREFIX "pvcs"
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
  _paraview_add_tests("paraview_add_client_server_render_tests"
    PREFIX "pvcrs"
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
  _paraview_add_tests("paraview_add_multi_client_tests"
    PREFIX "pvcs-multi-clients"
    _ENABLE_SUFFIX "_ENABLE_MULTI_CLIENT"
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
  _paraview_add_tests("paraview_add_multi_server_tests"
    PREFIX "pvcs-multi-servers"
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

  _paraview_add_tests("paraview_add_tile_display_tests"
    PREFIX "pvcs-tile-display"
    SUFFIX "-${width}x${height}"
    ENVIRONMENT
      PV_SHARED_WINDOW_SIZE=800x600
      SMTESTDRIVER_MPI_NUMPROCS=${_paraview_add_tile_display_cpu_count}
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

  _paraview_add_tests("paraview_add_cave_tests"
    PREFIX "pvcs-cave-${_config_name}"
    SUFFIX "-${num_ranks}"
    ENVIRONMENT
      PV_SHARED_WINDOW_SIZE=400x300
      SMTESTDRIVER_MPI_NUMPROCS=${num_ranks}
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
