if (NOT JSDUCK_EXECUTABLE)
  find_program(JSDUCK_EXECUTABLE
     NAME jsduck jsduck-4.8.0
     PATHS ENV JSDUCK_HOME
     PATH_SUFFIXES jsduck bin jsduck/bin
  )
endif()

find_package_handle_standard_args(JSDUCK DEFAULT_MSG JSDUCK_EXECUTABLE)
