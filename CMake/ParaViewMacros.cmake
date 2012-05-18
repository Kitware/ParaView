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
          
#########################################################################
# Function to compile a proto file to generate a .h and .cc file
# Arguments:
# out_cpp_file_variable: variable that gets set with the full path to output file
# in_proto_file: full path to input file (e.g. ${CMAKE_CURRENT_SOURCE_DIR}/foo.pb)

FUNCTION (protobuf_generate out_cpp_file in_proto_file)
  GET_FILENAME_COMPONENT(basename ${in_proto_file} NAME_WE)
  GET_FILENAME_COMPONENT(absolute ${in_proto_file} ABSOLUTE)
  GET_FILENAME_COMPONENT(path ${absolute} PATH)
  SET (out_file ${CMAKE_CURRENT_BINARY_DIR}/${basename}.pb.h)
  SET(${out_cpp_file}  ${out_file} PARENT_SCOPE)
  ADD_CUSTOM_COMMAND(
    OUTPUT ${out_file}
    COMMAND protoc_compiler
      --cpp_out=dllexport_decl=VTK_PROTOBUF_EXPORT:${CMAKE_CURRENT_BINARY_DIR}
      --proto_path ${path} ${absolute}
    DEPENDS ${in_proto_file} protoc_compiler
  )
ENDFUNCTION (protobuf_generate)

#########################################################################
# Function to generate header file from any file(s). Support ASCII as well as
# binary files.
# Usage:
# generate_header(name
#                 [PREFIX prefix_text]
#                 [SUFFIX suffix_text]
#                 [VARIABLE variablename]
#                 [BINARY]
#                 FILES <list-of-files>
# name :- name of the header file e.g. ${CMAKE_CURRENT_BINARY_DIR}/FooBar.h
# PREFIX :- (optional) when specified, used as the prefix for the generated
#           function/variable names.
# SUFFIX :- (optional) when specified, used as the suffix for the generated
#           function/variable names.
# BINARY :-(optional) when specified, all files are treated as binary and
#           encoded using base64.
# VARIABLE :- (optional) when specified, all the generate functions used to
#             access the compiled files are listed.
# FILES   :- list of files to compile in.
#------------------------------------------------------------------------------
function(generate_header name)
  pv_parse_arguments(arg
    "PREFIX;SUFFIX;VARIABLE;FILES"
    "BINARY"
    ${ARGN}
    )

  if (NOT PARAVIEW_PROCESS_XML_EXECUTABLE)
    MESSAGE (FATAL_ERROR
      "No PARAVIEW_PROCESS_XML_EXECUTABLE specified
      Could not locate kwProcessXML executable")
  endif ()

  set (function_names)
  set (input_files)
  set (have_xmls)
  foreach (input_file ${arg_FILES})
    get_filename_component(absolute_file "${input_file}" ABSOLUTE)
    get_filename_component(file_name "${absolute_file}" NAME_WE)
    list (APPEND function_names "${arg_PREFIX}${file_name}${arg_SUFFIX}")
    list (APPEND input_files "${absolute_file}")
    set (have_xmls TRUE)
  endforeach()

  set (base_64)
  if (arg_BINARY)
    set (base_64 "-base64")
  endif()

  if (have_xmls)
    add_custom_command(
      OUTPUT "${name}"
      COMMAND "${PARAVIEW_PROCESS_XML_EXECUTABLE}"
              ${base_64}
              ${name}
              \"${arg_PREFIX}\" 
              \"${arg_SUFFIX}\"
              \"${arg_SUFFIX}\"
              ${input_files}
      DEPENDS ${arg_FILES}
              ${PARAVIEW_PROCESS_XML_EXECUTABLE}
     ) 
  endif ()

  if (DEFINED arg_VARIABLE)
    set (${arg_VARIABLE} ${function_names} PARENT_SCOPE)
  endif()
endfunction()


# GENERATE_HTMLS_FROM_XMLS can be used to generate HTML files for
# from a given list of xml files that correspond to server manager xmls.
# ARGUMENTS:
# output_files: OUT: variables set to the output files
# xmls: IN : full pathnames to xml files.
# output_dir : IN : full path to output directory where to generate the htmls.
#------------------------------------------------------------------------------
function (generate_htmls_from_xmls output_files xmls gui_xmls output_dir)
  # create a string from the xmls list to pass
  # since this list needs to be passed as an argument, we cannot escape the ";".
  # generate_proxydocumentation.cmake has code to convert these strings back to
  # lists.
  set (xmls_string "")
  foreach (xml ${xmls})
    get_filename_component(xml "${xml}" ABSOLUTE)
    set (xmls_string "${xmls_string}${xml}+")
  endforeach()
  
  set (gui_xmls_string "")
  foreach (gui_xml ${gui_xmls})
    get_filename_component(gui_xml "${gui_xml}" ABSOLUTE)
    set (gui_xmls_string "${gui_xmls_string}${gui_xml}+")
  endforeach()

  set (all_xmls ${xmls} ${gui_xmls})
  list (GET all_xmls 0 first_xml)
  if (NOT first_xml)
    message(FATAL_ERROR "No xml specified!!!")
  endif()
  
  # extract the name from the first xml file. This is the name for temporary
  # file we use.
  get_filename_component(first_xml "${first_xml}" NAME)

  find_program(QT_XMLPATTERNS_EXECUTABLE
    xmlpatterns
    PATHS "${QT_BINARY_DIR}"
    DOC "xmlpatterns used to generate html from Proxy documentation.")
  mark_as_advanced(QT_XMLPATTERNS_EXECUTABLE)

  if (NOT EXISTS ${QT_XMLPATTERNS_EXECUTABLE})
    message(WARNING "Valid QT_XMLPATTERNS_EXECUTABLE not specified.")
  endif()

  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${first_xml}.xml"

    # process each html file to sperate it out into files for each proxy.
    COMMAND ${CMAKE_COMMAND}
            -Dxmlpatterns:FILEPATH=${QT_XMLPATTERNS_EXECUTABLE}
            -Dxml_to_xml_xsl:FILEPATH=${ParaView_CMAKE_DIR}/smxml_to_xml.xsl
            -Dxml_to_html_xsl:FILEPATH=${ParaView_CMAKE_DIR}/xml_to_html.xsl
            -Dxml_to_wiki_xsl:FILEPATH=${ParaView_CMAKE_DIR}/xml_to_wiki.xsl.in
            -Dinput_xmls:STRING=${xmls_string}
            -Dinput_gui_xmls:STRING=${gui_xmls_string}
            -Doutput_dir:PATH=${output_dir}
            -Doutput_file:FILEPATH=${CMAKE_CURRENT_BINARY_DIR}/${first_xml}.xml
            -P ${ParaView_CMAKE_DIR}/generate_proxydocumentation.cmake

    DEPENDS ${xmls}
            ${ParaView_CMAKE_DIR}/smxml_to_xml.xsl
            ${ParaView_CMAKE_DIR}/xml_to_html.xsl
            ${ParaView_CMAKE_DIR}/generate_proxydocumentation.cmake

    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"

    COMMENT "Generating Documentation HTMLs from xmls")

    set (dependencies ${dependencies}
          "${CMAKE_CURRENT_BINARY_DIR}/${first_xml}.xml")
  set (${output_files} ${dependencies} PARENT_SCOPE)
endfunction()

#------------------------------------------------------------------------------
# Function used to build a qhp (and qch) file. Adds a custom command to generate
# a ${DESTINATION_DIRECTORY}/${name}.qch.
# build_help_project(name 
#                    DESTINATION_DIRECTORY directory
#                    [DOCUMENTATION_SOURCE_DIR directory]
#                    [NAMESPACE namespacename (default:${name}.org)]
#                    [FOLDER virtualfoldername (default:${name})]
#                    [TABLE_OF_CONTENTS toc]
#                    [TABLE_OF_CONTENTS_FILE toc_file_name]
#                    [FILES relative filenames/wildcard-expressions]
#                   )
# name :- specifies the name for the qhp. The generated qhp file will be
#         ${DESTINATION_DIRECTORY}/${name}.qhp
# DESTINATION_DIRECTORY :- output-directory for the qhp file.
# DOCUMENTATION_SOURCE_DIR :- (optional) when specified, all files in this
#                             directory are copied over to the
#                             DESTINATION_DIRECTORY.
# NAMESPACE :- (optional; default=${name}.org") Namespace to use in qhp file.
# FOLDER :- (optional; default=${name}") virtual folder in qhp file.
# TABLE_OF_CONTENTS :- (optional) XML string <toc>..</toc> (see qhp file
#                      documentation). Used only when TABLE_OF_CONTENTS_FILE is
#                      not specified.
# TABLE_OF_CONTENTS_FILE :- file to read in to obtain the TABLE_OF_CONTENTS
# FILEPATTERNS :- (optional: default="*.*") list of files (names or wildcards) to list
#          in the qhp file. Note that these files/paths are relative to the
#          DESTINATION_DIRECTORY.
# DEPENDS :- (optional) targets or files that the qch generation target depends on.
#
# If neither TABLE_OF_CONTENTS or TABLE_OF_CONTENTS_FILE is specified, then the
# TOC is auto-generated.
#------------------------------------------------------------------------------
function(build_help_project name)
  pv_parse_arguments(arg
    "DESTINATION_DIRECTORY;DOCUMENTATION_SOURCE_DIR;NAMESPACE;FOLDER;TABLE_OF_CONTENTS;TABLE_OF_CONTENTS_FILE;FILEPATTERNS;DEPENDS"
    ""
    ${ARGN}
    )

  if (NOT DEFINED arg_DESTINATION_DIRECTORY)
    message(FATAL_ERROR "No DESTINATION_DIRECTORY specified in build_help_project()")
  endif()

  find_program(QT_HELP_GENERATOR
    qhelpgenerator
    PATHS "${QT_BINARY_DIR}"
    DOC "qhelpgenerator used to compile Qt help project files")
  mark_as_advanced(QT_HELP_GENERATOR)

  if (NOT EXISTS ${QT_HELP_GENERATOR})
    message(WARNING "Valid QT_HELP_GENERATOR not specified.")
  endif()

  # set default values for optional arguments.
  pv_set_if_not_set(arg_FILEPATTERNS "*.*")
  pv_set_if_not_set(arg_NAMESPACE "${name}.org")
  pv_set_if_not_set(arg_FOLDER "${name}")
  pv_set_if_not_set(arg_DEPENDS "")

  # if filename is specified, it takes precendence.
  # setup toc variable to refer to the TOC xml dom.
  if (DEFINED arg_TABLE_OF_CONTENTS_FILE)
    file(READ ${arg_TABLE_OF_CONTENTS_FILE} arg_TABLE_OF_CONTENTS)
  endif()

  set (qhp_filename ${arg_DESTINATION_DIRECTORY}/${name}.qhp)

  set (extra_args)
  if (arg_DOCUMENTATION_SOURCE_DIR)
    set (extra_args
      # copy all htmls from source to destination directory (same location where the
      # qhp file is present.
      COMMAND ${CMAKE_COMMAND} -E copy_directory
              "${arg_DOCUMENTATION_SOURCE_DIR}"
              "${arg_DESTINATION_DIRECTORY}"
      )
  endif()

  if (NOT DEFINED arg_TABLE_OF_CONTENTS)
    # sanitize arg_FILEPATTERNS since we pass it as a command line argument.
    string (REPLACE ";" "+" arg_FILEPATTERNS "${arg_FILEPATTERNS}")
    set (extra_args ${extra_args}

    # generate the toc at run-time.
    COMMAND ${CMAKE_COMMAND}
            -Doutput_file:FILEPATH=${qhp_filename}
            -Dfile_patterns:STRING="${arg_FILEPATTERNS}"
            -Dnamespace:STRING="${arg_NAMESPACE}"
            -Dfolder:PATH=${arg_FOLDER}
            -Dname:STRING="${name}"
            -P "${ParaView_CMAKE_DIR}/generate_qhp.cmake"
    )
  else ()
    # toc is provided, we'll just configure the file.
    set (files)
    foreach(filename ${arg_FILEPATTERNS})
      set (files "${files}<file>${filename}</file>\n")
    endforeach()

    configure_file(${ParaView_CMAKE_DIR}/build_help_project.qhp.in
      ${qhp_filename})
    list (APPEND arg_DEPENDS ${qhp_filename})
  endif()

  ADD_CUSTOM_COMMAND(
    OUTPUT ${arg_DESTINATION_DIRECTORY}/${name}.qch
    DEPENDS ${arg_DEPENDS}
            ${ParaView_CMAKE_DIR}/generate_qhp.cmake
  
    ${extra_args}

    # Now, compile the qhp file to generate the qch.
    COMMAND ${QT_HELP_GENERATOR}
            ${qhp_filename}
            -o ${arg_DESTINATION_DIRECTORY}/${name}.qch
  
    COMMENT "Compiling Qt help project ${name}.qhp"

    WORKING_DIRECTORY "${arg_DESTINATION_DIRECTORY}"
  )
endfunction(build_help_project)

macro(pv_set_link_interface_libs target)
  # if not lion then we need to set LINK_INTERFACE_LIBRARIES to reduce the number
  # of libraries we link against there is a limit of 253.
  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND CMAKE_SYSTEM_VERSION VERSION_LESS 11.0) 
    set_property(TARGET ${target}
      PROPERTY LINK_INTERFACE_LIBRARIES "${ARGN}")
  endif()
endmacro()
