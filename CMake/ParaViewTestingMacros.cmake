# Set up some common testing environment.
SET (CLIENT_EXECUTABLE  "\$<TARGET_FILE:paraview>")
# FIXME: need to verify that the above points to the paraview executable within
# the app bundle on Mac.

MACRO (process_args out_extra_args)
  SET (temp_args)
  IF (ACT_BASELINE_DIR)
    SET (temp_args "--test-baseline=${ACT_BASELINE_DIR}/${test_name}.png")
  ENDIF (ACT_BASELINE_DIR)
  IF (${test_name}_THRESHOLD)
    SET (temp_args ${temp_args} "--test-threshold=${${test_name}_THRESHOLD}")
  ENDIF (${test_name}_THRESHOLD)
  SET (${out_extra_args} ${${out_extra_args}} ${temp_args})
ENDMACRO (process_args)


#Determine how many tests are to be grouped.
SET (TEST_GROUP_SIZE 3)

FUNCTION (add_pv_test prefix skip_test_flag_suffix)
  PV_PARSE_ARGUMENTS(ACT "TEST_SCRIPTS;BASELINE_DIR;COMMAND;LOAD_PLUGIN;PLUGIN_PATH" "" ${ARGN})
  while (ACT_TEST_SCRIPTS)
    set (counter 0)
    set (extra_args)
    set (full_test_name)
    set (force_serial FALSE)

    while (${counter} LESS ${TEST_GROUP_SIZE})
      list(LENGTH ACT_TEST_SCRIPTS num_tests)
      if (num_tests)
        # pop test name from the top.
        list(GET ACT_TEST_SCRIPTS 0 test)
        list(REMOVE_AT ACT_TEST_SCRIPTS 0)
        GET_FILENAME_COMPONENT(test_name ${test} NAME_WE)

        # If this is "break" test, make sure that no other tests were already
        # added.
        if (${counter} GREATER 0)
          if (${test_name}_BREAK)
            set (counter 100000) # stop the group;
            # push the test back into the list.
            list(INSERT ACT_TEST_SCRIPTS 0 ${test})
          endif (${test_name}_BREAK)
        endif (${counter} GREATER 0)

        if (${counter} LESS 100000)
          if (NOT ${test_name}${skip_test_flag_suffix})
            set (full_test_name "${full_test_name}.${test_name}")
            set (extra_args ${extra_args} "--test-script=${test}")
            process_args(extra_args)
            if (DEFINED ${test_name}_USE_OLD_PANELS)
              set (extra_args ${extra_args} "--use-old-panels")
            endif ()
          endif (NOT ${test_name}${skip_test_flag_suffix})
        endif (${counter} LESS 100000)
      endif (num_tests)
      math(EXPR counter "${counter} + 1")
      if (DEFINED ${test_name}_BREAK)
        set (counter 100000) # stop the group.
      endif (DEFINED ${test_name}_BREAK)
      if (${test_name}_FORCE_SERIAL)
        set (force_serial TRUE)
      endif (${test_name}_FORCE_SERIAL)
    endwhile (${counter} LESS ${TEST_GROUP_SIZE})

    if (extra_args)
      ADD_TEST(NAME "${prefix}${full_test_name}"
        COMMAND smTestDriver
        --enable-bt
        ${ACT_COMMAND}
        ${extra_args}
        --exit
        )
      if (force_serial)
        set_tests_properties("${prefix}${full_test_name}" PROPERTIES RUN_SERIAL ON)
        message(STATUS "Running in serial \"${prefix}${full_test_name}\"")
      endif()

      # add the "PARAVIEW" label to the test properties. this allows for the user
      # to instruct cmake to run just the ParaView tests with the '-L' flag
      set_tests_properties("${prefix}${full_test_name}" PROPERTIES LABELS "PARAVIEW")
    endif (extra_args)
  endwhile (ACT_TEST_SCRIPTS)

ENDFUNCTION (add_pv_test)

# Add macro to support addition of paraview web tests
FUNCTION(add_pvweb_tests prefix)
  PV_PARSE_ARGUMENTS(ACT
    "APP;TEST_SCRIPTS;BASELINE_DIR;COMMAND;ARGS;SERVER;DEPEND_MODS;BROWSER"
    ""
    ${ARGN})

  set(go_ahead_with_tests TRUE)

  # First check that all requested Python modules are present
  if(DEFINED ACT_DEPEND_MODS)
    foreach(module_name ${ACT_DEPEND_MODS})
      find_python_module(${module_name} got_${module_name})
      if(NOT ${got_${module_name}})
        message(STATUS "Missing Python module: ${module_name}")
        set(go_ahead_with_tests FALSE)
      endif()
    endforeach()
  endif()

  # If this batch of tests has baseline images that need to be
  # compared against, make sure to include the baseline image
  # directory.  Otherwise, we will just leave this variable blank.
  if(DEFINED ACT_BASELINE_DIR)
    set(BASELINE_IMG_DIR "--baseline-img-dir;${ACT_BASELINE_DIR}")
  else()
    set(BASELINE_IMG_DIR "")
  endif()

  # Check for any args that we got
  if(DEFINED ACT_ARGS)
    set(ARGS ${ACT_ARGS})
  else()
    set(ARGS "")
  endif()

  # If there are browsers specified, we will iterate over them
  # and run the test scripts on each one.  If there isn't a
  # browser specified, we'll make one called "no-browser" to
  # keep the code clean.
  if (NOT DEFINED ACT_BROWSER)
    set(ACT_BROWSER "nobrowser")
  else()
    # There is at least one browser test requested, so we need
    # to find the selenium drivers
    find_package(SeleniumDrivers)
  endif()

  while(ACT_BROWSER)
    # Pull another browser off the list
    list(GET ACT_BROWSER 0 browser)
    list(REMOVE_AT ACT_BROWSER 0)

    set(browser_${browser}_ok TRUE)

    # In this section, we make sure that the needed browser driver is
    # installed on the system.  Some tests might not require selenium
    # or an actual browser.
    if(${browser} STREQUAL "nobrowser")
      # Anything you need to do in the case of no browser.  Currently, nothing.
    else()
      if(${browser} STREQUAL "chrome")
        if(${CHROMEDRIVER_EXECUTABLE} MATCHES "NOTFOUND")
          set(browser_${browser}_ok FALSE)
        endif()
      elseif(${browser} STREQUAL "firefox")
        if(${FIREFOXDRIVER_EXTENSION} MATCHES "NOTFOUND")
          set(browser_${browser}_ok FALSE)
        endif()
      elseif(${browser} STREQUAL "internet_explorer")
        if(${IEDRIVER_EXECUTABLE} MATCHES "NOTFOUND")
          set(browser_${browser}_ok FALSE)
        endif()
      elseif(${browser} STREQUAL "safari")
        if(${SAFARIDRIVER_EXTENSION} MATCHES "NOTFOUND")
          set(browser_${browser}_ok FALSE)
        endif()
      endif()
    endif()

    # If we made it through checks for python modules and browser driver
    # then we are ready to add the tests.
    if(${go_ahead_with_tests} AND ${browser_${browser}_ok})

      # Create a copy of the scripts list so we keep the original intact
      set(TEST_SCRIPTS_LIST ${ACT_TEST_SCRIPTS})

      while (TEST_SCRIPTS_LIST)
        # pop test script path from the top of the list
        list(GET TEST_SCRIPTS_LIST 0 test_path)
        list(REMOVE_AT TEST_SCRIPTS_LIST 0)
        GET_FILENAME_COMPONENT(script_name ${test_path} NAME_WE)

        set(short_script_name ${script_name})

        # Use a regular expression to remove the first 4 batches of
        # underscore-separated character strings
        if(${script_name} MATCHES "^[^_]+_[^_]+_[^_]+_[^_]+_(.+)")
          set(short_script_name ${CMAKE_MATCH_1})
        endif()

        set(test_name "${prefix}-${browser}.${ACT_APP}-${short_script_name}")
        set(test_image_file_name "${test_name}.png")

        add_test(NAME ${test_name}
          COMMAND ${ACT_COMMAND}
                  ${ACT_SERVER}
                  --content ${ParaView_BINARY_DIR}/www
                  --data-dir ${PARAVIEW_DATA_ROOT}/Data
                  --port 8080
                  ${ARGS}
                  ${BASELINE_IMG_DIR}
                  --run-test-script ${test_path}
                  --test-use-browser ${browser}
                  --temporary-directory ${ParaView_BINARY_DIR}/Testing/Temporary
                  --test-image-file-name ${test_image_file_name}
                  )
        set_tests_properties(${test_name} PROPERTIES LABELS "PARAVIEW")
      endwhile()
    else()
      message(STATUS "${prefix}-${ACT_APP}-${browser} tests disabled, missing requirements")
    endif()
  endwhile()
ENDFUNCTION()

# ----------------------------------------------------------------------------
# Test functions
FUNCTION (add_client_tests prefix)
  PV_EXTRACT_CLIENT_SERVER_ARGS(${ARGN})

  add_pv_test(${prefix} "_DISABLE_C"
    COMMAND --client ${CLIENT_EXECUTABLE}
            --enable-bt
            -dr
            ${CLIENT_SERVER_ARGS}
            --test-directory=${PARAVIEW_TEST_DIR}
    ${ARGN})
ENDFUNCTION (add_client_tests)

FUNCTION (add_client_server_tests prefix)
  PV_EXTRACT_CLIENT_SERVER_ARGS(${ARGN})
  add_pv_test(${prefix} "_DISABLE_CS"
    COMMAND
       --server $<TARGET_FILE:pvserver>
       --enable-bt
         ${CLIENT_SERVER_ARGS}
       --client ${CLIENT_EXECUTABLE}
       --enable-bt
         ${CLIENT_SERVER_ARGS}
       -dr
       --test-directory=${PARAVIEW_TEST_DIR}
    ${ARGN})
ENDFUNCTION (add_client_server_tests)

FUNCTION (add_client_render_server_tests prefix)
  PV_EXTRACT_CLIENT_SERVER_ARGS(${ARGN})
  add_pv_test(${prefix} "_DISABLE_CRS"
    COMMAND
       --data-server $<TARGET_FILE:pvdataserver>
       --enable-bt
            ${CLIENT_SERVER_ARGS}
       --render-server $<TARGET_FILE:pvrenderserver>
       --enable-bt
            ${CLIENT_SERVER_ARGS}
       --client ${CLIENT_EXECUTABLE}
       --enable-bt
            ${CLIENT_SERVER_ARGS}
       -dr
       --test-directory=${PARAVIEW_TEST_DIR}
    ${ARGN})
ENDFUNCTION (add_client_render_server_tests)

FUNCTION(add_multi_client_tests prefix)
  PV_PARSE_ARGUMENTS(ACT "TEST_SCRIPTS;BASELINE_DIR" "" ${ARGN})

  foreach (test_script ${ACT_TEST_SCRIPTS})
    get_filename_component(test_name ${test_script} NAME_WE)
    if (${test_name}_ENABLE_COLLAB)
      set (extra_args)
      set (use_old_panels)
      process_args(extra_args)
      if (DEFINED ${test_name}_USE_OLD_PANELS)
        set (use_old_panels "--use-old-panels")
      endif ()

      add_test(NAME "${prefix}.${test_name}"
        COMMAND smTestDriver
        --test-multi-clients
        --server $<TARGET_FILE:pvserver>
        --enable-bt

        --client ${CLIENT_EXECUTABLE}
        --enable-bt
        -dr
        --test-directory=${PARAVIEW_TEST_DIR}
        --test-script=${test_script}
        --test-master
        ${use_old_panels}
        --exit

        --client ${CLIENT_EXECUTABLE}
        --enable-bt
        -dr
        --test-directory=${PARAVIEW_TEST_DIR}
        --test-slave
        ${extra_args}
        --exit
        )
      if (${test_name}_FORCE_SERIAL)
        set_tests_properties("${prefix}.${test_name}" PROPERTIES RUN_SERIAL ON)
        message(STATUS "Running in serial \"${prefix}.${test_name}\"")
      endif (${test_name}_FORCE_SERIAL)

      set_tests_properties("${prefix}.${test_name}" PROPERTIES LABELS "PARAVIEW")
    endif()
  endforeach(test_script)
ENDFUNCTION(add_multi_client_tests)

FUNCTION(add_multi_server_tests prefix nbServers)
  PV_PARSE_ARGUMENTS(ACT "TEST_SCRIPTS;BASELINE_DIR" "" ${ARGN})

  foreach (test_script ${ACT_TEST_SCRIPTS})
    get_filename_component(test_name ${test_script} NAME_WE)
      set (extra_args)
      process_args(extra_args)
      add_test(NAME "${prefix}.${test_name}"
        COMMAND smTestDriver
        --test-multi-servers ${nbServers}
        --server $<TARGET_FILE:pvserver>
        --enable-bt

        --client ${CLIENT_EXECUTABLE}
        --enable-bt
        -dr
        --test-directory=${PARAVIEW_TEST_DIR}
        --test-script=${test_script}
        ${extra_args}
        --exit
        )
      set_tests_properties("${prefix}.${test_name}" PROPERTIES LABELS "PARAVIEW")
  endforeach(test_script)
ENDFUNCTION(add_multi_server_tests)

FUNCTION (add_tile_display_tests prefix tdx tdy )
  PV_PARSE_ARGUMENTS(ACT "TEST_SCRIPTS;BASELINE_DIR;LOAD_PLUGIN;PLUGIN_PATH" "" ${ARGN})
  PV_EXTRACT_CLIENT_SERVER_ARGS(${ARGN})

  MATH(EXPR REQUIRED_CPU '${tdx}*${tdy}-1') # -1 is for LESS
  if (${PARAVIEW_USE_MPI})
    if (NOT DEFINED VTK_MPI_MAX_NUMPROCS)
      set (VTK_MPI_MAX_NUMPROCS ${MPIEXEC_MAX_NUMPROCS})
    endif()
    if (${REQUIRED_CPU} LESS ${VTK_MPI_MAX_NUMPROCS})
      foreach (test_script ${ACT_TEST_SCRIPTS})

        get_filename_component(test_name ${test_script} NAME_WE)
        set (extra_args)
        process_args(extra_args)
        add_test(NAME "${prefix}-${tdx}x${tdy}.${test_name}"
            COMMAND smTestDriver
            --test-tiled ${tdx} ${tdy}
            --server $<TARGET_FILE:pvserver>
            --enable-bt
            ${CLIENT_SERVER_ARGS}
            --client ${CLIENT_EXECUTABLE}
            --enable-bt
            ${CLIENT_SERVER_ARGS}
            -dr
            --test-directory=${PARAVIEW_TEST_DIR}
            --test-script=${test_script}
            --tile-image-prefix=${PARAVIEW_TEST_DIR}/${test_name}

            ${extra_args}
            --exit
            )
        set_property(TEST "${prefix}-${tdx}x${tdy}.${test_name}"
                     PROPERTY ENVIRONMENT "PV_ICET_WINDOW_BORDERS=1")
        set_tests_properties("${prefix}-${tdx}x${tdy}.${test_name}" PROPERTIES RUN_SERIAL ON)
        set_tests_properties("${prefix}-${tdx}x${tdy}.${test_name}" PROPERTIES LABELS "PARAVIEW")
      endforeach(test_script)
    endif(${REQUIRED_CPU} LESS ${VTK_MPI_MAX_NUMPROCS})
  endif()
ENDFUNCTION (add_tile_display_tests)
