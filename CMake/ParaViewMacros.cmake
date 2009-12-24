# File defining miscellaneous macros

#------------------------------------------------------------------------------
# GENERATE_QT_RESOURCE_FROM_FILES can be used to generate a Qt resource file
# from a given set of files.
# ARGUMENTS:
# resource_file: IN : full pathname of the qrc file to generate. 
# resource_prefix: IN : the name used in the "prefix" attribute for the
#                       generated qrc file.
# file_list: IN : list of files to be added into the resource file.
#------------------------------------------------------------------------------
MACRO(GENERATE_QT_RESOURCE_FROM_FILES resource_file resource_prefix file_list)
  SET (pq_resource_file_contents "<RCC>\n  <qresource prefix=\"${resource_prefix}\">\n")
  GET_FILENAME_COMPONENT(current_directory ${resource_file} PATH)
  FOREACH (resource ${file_list})
    GET_FILENAME_COMPONENT(alias ${resource} NAME)
    GET_FILENAME_COMPONENT(resource ${resource} ABSOLUTE)
    FILE(RELATIVE_PATH resource "${current_directory}" "${resource}")
    FILE(TO_NATIVE_PATH "${resource}" resource)
    SET (pq_resource_file_contents
      "${pq_resource_file_contents}    <file alias=\"${alias}\">${resource}</file>\n")
  ENDFOREACH (resource)
  SET (pq_resource_file_contents
    "${pq_resource_file_contents}  </qresource>\n</RCC>\n")

  # Generate the resource file.
  FILE (WRITE "${resource_file}" "${pq_resource_file_contents}")
ENDMACRO(GENERATE_QT_RESOURCE_FROM_FILES)

#----------------------------------------------------------------------------
# PV_PARSE_ARGUMENTS is a macro useful for writing macros that take a key-word
# style arguments.
#----------------------------------------------------------------------------
MACRO(PV_PARSE_ARGUMENTS prefix arg_names option_names)
  SET(DEFAULT_ARGS)
  FOREACH(arg_name ${arg_names})    
    SET(${prefix}_${arg_name})
  ENDFOREACH(arg_name)
  FOREACH(option ${option_names})
    SET(${prefix}_${option} FALSE)
  ENDFOREACH(option)

  SET(current_arg_name DEFAULT_ARGS)
  SET(current_arg_list)
  FOREACH(arg ${ARGN})            
    SET(larg_names ${arg_names})    
    LIST(FIND larg_names "${arg}" is_arg_name)                   
    IF (is_arg_name GREATER -1)
      SET(${prefix}_${current_arg_name} ${current_arg_list})
      SET(current_arg_name ${arg})
      SET(current_arg_list)
    ELSE (is_arg_name GREATER -1)
      SET(loption_names ${option_names})    
      LIST(FIND loption_names "${arg}" is_option)            
      IF (is_option GREATER -1)
       SET(${prefix}_${arg} TRUE)
      ELSE (is_option GREATER -1)
       SET(current_arg_list ${current_arg_list} ${arg})
      ENDIF (is_option GREATER -1)
    ENDIF (is_arg_name GREATER -1)
  ENDFOREACH(arg)
  SET(${prefix}_${current_arg_name} ${current_arg_list})
ENDMACRO(PV_PARSE_ARGUMENTS)

#----------------------------------------------------------------------------
# Macro for setting values if a user did not overwrite them
#----------------------------------------------------------------------------
MACRO(pv_set_if_not_set name value)
  IF(NOT DEFINED "${name}")
    SET(${name} "${value}")
  ENDIF(NOT DEFINED "${name}")
ENDMACRO(pv_set_if_not_set)
