# ---------------------------------------------------------------------------
# KWWidgets_CREATE_RC_FILE
# This macro can be used to create a Win32 .rc file out of the resource
# template found in Resources/KWWidgets.rc.in. Such a Win32 resource file
# can be added to the list of source files associated to a specific
# application/executable, and can be used to customize both the 16x16 and
# 32x32 icons, as well as the informations that are displayed in the "Version"
# tab of its properties panel.
#
# This macro accepts parameters as arg/value pairs or as a single arg if
# the arg is described as boolean (same as setting the arg to 1). The
# args can be specificied in any order and some of them are optionals.
#
# Required arguments:
# RC_FILENAME (filename): pathname of the resource file to create
#    Default to "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rc" if not found.
#
# Optional arguments:
# RC_ICON_BASENAME (path): path that will be used as basename for the 16x16
#    and 32x32 icons. The full pathname to the 16x16 icon is set to this
#    basename suffixed by "Icon16.ico" (Icon32.ico for the 32x32).
#    Default to "Resources/KWWidgets" if not found.
# RC_MAJOR_VERSION (string): major version number of the application
#    Default to 1 if not found.
# RC_MINOR_VERSION (string): minon version number of the application
#    Default to 0 if not found.
# RC_APPLICATION_NAME (string): the application name
#    Default to ${PROJECT_NAME} if not found.
# RC_APPLICATION_FILENAME (basename): the basename of application file, i.e.
#    its name without path or file extension.
#    Default to ${RC_APPLICATION_NAME}
# RC_COMPANY_NAME (string): the name of the company associated to the app
#    Default to "Unknown" if not found.
# RC_COPYRIGHT_YEAR (string): the copyright year(s) that apply to that 
#    application (say, 2005, or 2003-2006)
#    Default to "2006" if not found.

MACRO(KWWidgets_CREATE_RC_FILE)

  SET(notset_value             "__not_set__")

  # Provide some reasonable defaults

  SET(RC_FILENAME             "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rc")
  SET(RC_ICON_BASENAME        "Resources/KWWidgets")
  SET(RC_MAJOR_VERSION        1)
  SET(RC_MINOR_VERSION        0)
  SET(RC_APPLICATION_NAME     ${PROJECT_NAME})
  SET(RC_APPLICATION_FILENAME ${notset_value})
  SET(RC_COMPANY_NAME         "Unknown")
  SET(RC_COPYRIGHT_YEAR       2006)

  # Parse the arguments

  SET(valued_parameter_names "^(___RC_FILENAME|___RC_ICON_BASENAME|___RC_MAJOR_VERSION|___RC_MINOR_VERSION|___RC_APPLICATION_NAME|___RC_APPLICATION_FILENAME|___RC_COMPANY_NAME|___RC_COPYRIGHT_YEAR)$")
  SET(boolean_parameter_names "^$")
  SET(list_parameter_names "^$")

  SET(next_arg_should_be_value 0)
  SET(prev_arg_was_boolean 0)
  SET(prev_arg_was_list 0)
  SET(unknown_parameters)
  
  STRING(REGEX REPLACE ";;" ";FOREACH_FIX;" parameter_list "${ARGV}")
  FOREACH(arg ${parameter_list})

    IF("${arg}" STREQUAL "FOREACH_FIX")
      SET(arg "")
    ENDIF("${arg}" STREQUAL "FOREACH_FIX")

    SET(___arg "___${arg}")
    SET(matches_valued 0)
    IF("${___arg}" MATCHES ${valued_parameter_names})
      SET(matches_valued 1)
    ENDIF("${___arg}" MATCHES ${valued_parameter_names})

    SET(matches_boolean 0)
    IF("${___arg}" MATCHES ${boolean_parameter_names})
      MESSAGE("bool: ${arg}")
      SET(matches_boolean 1)
    ENDIF("${___arg}" MATCHES ${boolean_parameter_names})

    SET(matches_list 0)
    IF("${___arg}" MATCHES ${list_parameter_names})
      MESSAGE("list: ${arg}")
      SET(matches_list 1)
    ENDIF("${___arg}" MATCHES ${list_parameter_names})
      
    IF(matches_valued OR matches_boolean OR matches_list)
      IF(prev_arg_was_boolean)
        SET(${prev_arg_name} 1)
      ELSE(prev_arg_was_boolean)
        IF(next_arg_should_be_value AND NOT prev_arg_was_list)
          MESSAGE(FATAL_ERROR 
            "Found ${arg} instead of value for ${prev_arg_name}")
        ENDIF(next_arg_should_be_value AND NOT prev_arg_was_list)
      ENDIF(prev_arg_was_boolean)
      SET(next_arg_should_be_value 1)
      SET(prev_arg_was_boolean ${matches_boolean})
      SET(prev_arg_was_list ${matches_list})
      SET(prev_arg_name ${arg})
    ELSE(matches_valued OR matches_boolean OR matches_list)
      IF(next_arg_should_be_value)
        IF(prev_arg_was_boolean)
          IF(NOT "${arg}" STREQUAL "1" AND NOT "${arg}" STREQUAL "0")
            MESSAGE(FATAL_ERROR 
              "Found ${arg} instead of 0 or 1 for ${prev_arg_name}")
          ENDIF(NOT "${arg}" STREQUAL "1" AND NOT "${arg}" STREQUAL "0")
        ENDIF(prev_arg_was_boolean)
        IF(prev_arg_was_list)
          SET(${prev_arg_name} ${${prev_arg_name}} ${arg})
        ELSE(prev_arg_was_list)
          SET(${prev_arg_name} ${arg})
          SET(next_arg_should_be_value 0)
        ENDIF(prev_arg_was_list)
      ELSE(next_arg_should_be_value)
        SET(unknown_parameters ${unknown_parameters} ${arg})
      ENDIF(next_arg_should_be_value)
      SET(prev_arg_was_boolean 0)
    ENDIF(matches_valued OR matches_boolean OR matches_list)

  ENDFOREACH(arg)

  IF(next_arg_should_be_value)
    IF(prev_arg_was_boolean)
      SET(${prev_arg_name} 1)
    ELSE(prev_arg_was_boolean)
      MESSAGE(FATAL_ERROR "Missing value for ${prev_arg_name}")
    ENDIF(prev_arg_was_boolean)
  ENDIF(next_arg_should_be_value)
  IF(unknown_parameters)
    MESSAGE(FATAL_ERROR "Unknown parameter(s): ${unknown_parameters}")
  ENDIF(unknown_parameters)

  # Fix some defaults

  IF(${RC_APPLICATION_FILENAME} STREQUAL ${notset_value})
    SET(RC_APPLICATION_FILENAME ${RC_APPLICATION_NAME})
  ENDIF(${RC_APPLICATION_FILENAME} STREQUAL ${notset_value})

  # Create the resource file

  INCLUDE_DIRECTORIES(${VTK_TK_RESOURCES_DIR})
  CONFIGURE_FILE(${KWWidgets_RESOURCES_DIR}/KWWidgets.rc.in ${RC_FILENAME})

ENDMACRO(KWWidgets_CREATE_RC_FILE)

