if (NOT JSDUCK_EXECUTABLE)
  find_program(JSDUCK_EXECUTABLE jsduck)
endif()

find_package_handle_standard_args(JSDUCK DEFAULT_MSG JSDUCK_EXECUTABLE)
