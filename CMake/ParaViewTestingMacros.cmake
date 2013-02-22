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
  PV_PARSE_ARGUMENTS(ACT "TEST_SCRIPTS;BASELINE_DIR;COMMAND" "" ${ARGN})
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
            if (NOT DEFINED ${test_name}_USE_NEW_PANELS)
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


FUNCTION (add_client_tests prefix)
  add_pv_test(${prefix} "_DISABLE_C" 
    COMMAND --client ${CLIENT_EXECUTABLE}
            -dr
            --disable-light-kit
            --test-directory=${PARAVIEW_TEST_DIR}
    ${ARGN})
ENDFUNCTION (add_client_tests)

FUNCTION (add_client_server_tests prefix)
  add_pv_test(${prefix} "_DISABLE_CS"
    COMMAND
       --server $<TARGET_FILE:pvserver>
       --client ${CLIENT_EXECUTABLE}
       -dr
       --disable-light-kit
       --test-directory=${PARAVIEW_TEST_DIR}
    ${ARGN})
ENDFUNCTION (add_client_server_tests)

FUNCTION (add_client_render_server_tests prefix)
  add_pv_test(${prefix} "_DISABLE_CRS"
    COMMAND
       --data-server $<TARGET_FILE:pvdataserver>
       --render-server $<TARGET_FILE:pvrenderserver>
       --client ${CLIENT_EXECUTABLE}
       -dr
       --disable-light-kit
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
      if (NOT DEFINED ${test_name}_USE_NEW_PANELS)
        set (use_old_panels "--use-old-panels")
      endif ()

      add_test(NAME "${prefix}.${test_name}"
        COMMAND smTestDriver
        --test-multi-clients
        --server $<TARGET_FILE:pvserver>

        --client ${CLIENT_EXECUTABLE}
        -dr
        --disable-light-kit
        --test-directory=${PARAVIEW_TEST_DIR}
        --test-script=${test_script}
        --test-master
        ${use_old_panels}
        --exit

        --client ${CLIENT_EXECUTABLE}
        -dr
        --disable-light-kit
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

        --client ${CLIENT_EXECUTABLE}
        -dr
        --disable-light-kit
        --test-directory=${PARAVIEW_TEST_DIR}
        --test-script=${test_script}
        ${extra_args}
        --exit
        )
      set_tests_properties("${prefix}.${test_name}" PROPERTIES LABELS "PARAVIEW")
  endforeach(test_script)
ENDFUNCTION(add_multi_server_tests)

FUNCTION (add_tile_display_tests prefix tdx tdy )
  PV_PARSE_ARGUMENTS(ACT "TEST_SCRIPTS;BASELINE_DIR" "" ${ARGN})


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

            --client ${CLIENT_EXECUTABLE}
            -dr
            --disable-light-kit
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
