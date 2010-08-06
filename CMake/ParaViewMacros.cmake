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

#----------------------------------------------------------------------------
# When installing system libraries, on non-windows machines, the CMake variable
# pointing to the library may be a sym-link, in which case we don't simply want
# to install the symlink, but the actual library. This macro takes care of that.
# Use it for installing system libraries. Call this only on unix boxes.
FUNCTION (pv_install_library libpath dest component)
  IF (NOT WIN32)
    GET_FILENAME_COMPONENT(dir_tmp ${libpath} PATH)
    SET(name_tmp)
    # libs symlinks are always named lib.*.dylib on mac
    # libs symlinks are always named lib.so.* on linux
    IF (APPLE)
      GET_FILENAME_COMPONENT(name_tmp ${libpath} NAME_WE)
      FILE(GLOB lib_list "${dir_tmp}/${name_tmp}*")
    ELSE (APPLE)
      GET_FILENAME_COMPONENT(dir_tmp ${libpath} PATH)
      GET_FILENAME_COMPONENT(name_tmp ${libpath} NAME)
      FILE(GLOB lib_list RELATIVE "${dir_tmp}" "${libpath}*")
    ENDIF (APPLE)
    INSTALL(CODE "
          MESSAGE(STATUS \"Installing ${name_tmp}\")
          EXECUTE_PROCESS (WORKING_DIRECTORY ${dir_tmp}
               COMMAND tar c ${lib_list}
               COMMAND tar -xC \${CMAKE_INSTALL_PREFIX}/${dest})
               " COMPONENT ${component})
  ENDIF (NOT WIN32)
ENDFUNCTION (pv_install_library)

#----------------------------------------------------------------------------
# Function for adding an executable with support for shared forwarding.
# Typically, one just uses ADD_EXECUTABLE to add an executable target. However
# on linuxes when rpath is off, and shared libararies are on, to over come the
# need for setting the LD_LIBRARY_PATH, we use shared-forwarding. This macro
# makes it easier to employ shared forwarding if needed. 
# ARGUMENTS:
# out_real_exe_suffix -- (out) suffix to be added to the exe-target to locate the
#                     real executable target when shared forwarding is employed.
#                     This is empty when shared forwarding is not needed.
# exe_name        -- (in)  exe target name i.e. the first argument to
#                    ADD_EXECUTABLE.
# Any remaining arguments are simply passed on to the ADD_EXECUTABLE call.
# While writing install rules for this executable. One typically does the
# following.
#   INSTALL(TARGETS exe_name
#           DESTINATION "bin"
#           COMPONENT Runtime)
#   IF (pv_exe_suffix)
#     # Shared forwarding enabled.
#     INSTALL(TARGETS exe_name${out_real_exe_suffix}
#             DESTINATION "lib"
#             COMPONENT Runtime)
#   ENDIF (pv_exe_suffix)
#----------------------------------------------------------------------------
FUNCTION (add_executable_with_forwarding
            out_real_exe_suffix
            exe_name
            )
  if (NOT DEFINED PV_INSTALL_LIB_DIR)
    MESSAGE(FATAL_ERROR
      "PV_INSTALL_LIB_DIR variable must be set before calling add_executable_with_forwarding"
    )
  endif (NOT DEFINED PV_INSTALL_LIB_DIR)

  add_executable_with_forwarding2(out_var "" "" 
    ${PV_INSTALL_LIB_DIR}
    ${exe_name} ${ARGN})
  set (${out_real_exe_suffix} "${out_var}" PARENT_SCOPE)
ENDFUNCTION(add_executable_with_forwarding)

#----------------------------------------------------------------------------
FUNCTION (add_executable_with_forwarding2
            out_real_exe_suffix
            extra_build_dirs
            extra_install_dirs
            install_lib_dir
            exe_name
            )

  SET(mac_bundle)
  IF (APPLE)
    set (largs ${ARGN})
    LIST (FIND largs "MACOSX_BUNDLE" mac_bundle_index)
    IF (mac_bundle_index GREATER -1)
      SET (mac_bundle TRUE)
    ENDIF (mac_bundle_index GREATER -1)
  ENDIF (APPLE)

  SET(PV_EXE_SUFFIX)
  IF (BUILD_SHARED_LIBS AND NOT mac_bundle)
    IF(NOT WIN32)
      SET(exe_output_path ${EXECUTABLE_OUTPUT_PATH})
      IF (NOT EXECUTABLE_OUTPUT_PATH)
        SET (exe_output_path ${CMAKE_BINARY_DIR})
      ENDIF (NOT EXECUTABLE_OUTPUT_PATH)
      SET(PV_EXE_SUFFIX -real)
      SET(PV_FORWARD_DIR_BUILD "${exe_output_path}")
      SET(PV_FORWARD_DIR_INSTALL "../${install_lib_dir}")
      SET(PV_FORWARD_PATH_BUILD "\"${PV_FORWARD_DIR_BUILD}\"")
      SET(PV_FORWARD_PATH_INSTALL "\"${PV_FORWARD_DIR_INSTALL}\"")
      FOREACH(dir ${extra_build_dirs})
        SET (PV_FORWARD_PATH_BUILD "${PV_FORWARD_PATH_BUILD},\"${dir}\"")
      ENDFOREACH(dir)
      FOREACH(dir ${extra_install_dirs})
        SET (PV_FORWARD_PATH_INSTALL "${PV_FORWARD_PATH_INSTALL},\"${dir}\"")
      ENDFOREACH(dir)

      SET(PV_FORWARD_EXE ${exe_name})
      CONFIGURE_FILE(
        ${ParaView_CMAKE_DIR}/pv-forward.c.in
        ${CMAKE_CURRENT_BINARY_DIR}/${exe_name}-forward.c
        @ONLY IMMEDIATE)
      add_executable(${exe_name}
        ${CMAKE_CURRENT_BINARY_DIR}/${exe_name}-forward.c)
      set_target_properties(${exe_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/launcher)
      ADD_DEPENDENCIES(${exe_name} ${exe_name}${PV_EXE_SUFFIX})
    ENDIF(NOT WIN32)
  ENDIF (BUILD_SHARED_LIBS AND NOT mac_bundle)

  add_executable(${exe_name}${PV_EXE_SUFFIX} ${ARGN})
  set_target_properties(${exe_name}${PV_EXE_SUFFIX} PROPERTIES
        OUTPUT_NAME ${exe_name})

  set (${out_real_exe_suffix} "${PV_EXE_SUFFIX}" PARENT_SCOPE)
ENDFUNCTION (add_executable_with_forwarding2)
          
