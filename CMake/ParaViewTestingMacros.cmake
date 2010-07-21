# Set up some common testing environment.
SET (CLIENT_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/paraview)
IF (Q_WS_MAC)
  SET(CLIENT_EXECUTABLE
    ${EXECUTABLE_OUTPUT_PATH}/paraview.app/Contents/MacOS/paraview)
ENDIF (Q_WS_MAC)


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
  PV_PARSE_ARGUMENTS(ACT "TEST_SCRIPTS;BASELINE_DIR;COMMAND" "PARALLEL" ${ARGN})
  while (ACT_TEST_SCRIPTS)
    set (counter 0)
    set (extra_args)
    set (full_test_name)
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
          endif (NOT ${test_name}${skip_test_flag_suffix})
        endif (${counter} LESS 100000)
      endif (num_tests)
      math(EXPR counter "${counter} + 1")
      if (DEFINED ${test_name}_BREAK)
        set (counter 100000) # stop the group.
      endif (DEFINED ${test_name}_BREAK)
    endwhile (${counter} LESS ${TEST_GROUP_SIZE})

    if (extra_args)
      ADD_TEST("${prefix}${full_test_name}"
        ${PARAVIEW_SMTESTDRIVER_EXECUTABLE} 
        ${ACT_COMMAND}
        ${extra_args}
        --exit
        )
      if(${ACT_PARALLEL} STREQUAL "FALSE")
        set_tests_properties("${prefix}${full_test_name}" PROPERTIES RUN_SERIAL ON)
      endif()
      #if (ACT_PARALLEL
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
       --server ${PARAVIEW_SERVER_EXECUTABLE}
       --client ${CLIENT_EXECUTABLE}
       -dr
       --disable-light-kit
       --server=testserver
       --test-directory=${PARAVIEW_TEST_DIR}
    ${ARGN})
ENDFUNCTION (add_client_server_tests)

FUNCTION (add_client_render_server_tests prefix)
  add_pv_test(${prefix} "_DISABLE_CRS"
    COMMAND
       --data-server ${PARAVIEW_DATA_SERVER_EXECUTABLE}
       --render-server ${PARAVIEW_RENDER_SERVER_EXECUTABLE}
       --client ${CLIENT_EXECUTABLE}
       -dr
       --disable-light-kit
       --server=testserver-dsrs
       --test-directory=${PARAVIEW_TEST_DIR}
    ${ARGN})
ENDFUNCTION (add_client_render_server_tests)
