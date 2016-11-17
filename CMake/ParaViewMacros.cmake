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
FUNCTION(GENERATE_QT_RESOURCE_FROM_FILES resource_file resource_prefix file_list)
  SET (pq_resource_file_contents "<RCC>\n  <qresource prefix=\"${resource_prefix}\">\n")
  FOREACH (resource ${file_list})
    GET_FILENAME_COMPONENT(alias ${resource} NAME)
    GET_FILENAME_COMPONENT(resource ${resource} ABSOLUTE)
    GET_FILENAME_COMPONENT(resource ${resource} REALPATH)
    FILE(TO_NATIVE_PATH "${resource}" resource)
    SET (pq_resource_file_contents
      "${pq_resource_file_contents}    <file alias=\"${alias}\">${resource}</file>\n")
  ENDFOREACH ()
  SET (pq_resource_file_contents
    "${pq_resource_file_contents}  </qresource>\n</RCC>\n")

  # Generate the resource file.
  set (CMAKE_CONFIGURABLE_FILE_CONTENT "${pq_resource_file_contents}")
  configure_file(${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in
                 "${resource_file}")
  unset (CMAKE_CONFIGURABLE_FILE_CONTENT)
ENDFUNCTION()

#----------------------------------------------------------------------------
# PV_PARSE_ARGUMENTS is a macro useful for writing macros that take a key-word
# style arguments.
#----------------------------------------------------------------------------
MACRO(PV_PARSE_ARGUMENTS prefix arg_names option_names)
  SET(DEFAULT_ARGS)
  FOREACH(arg_name ${arg_names})    
    SET(${prefix}_${arg_name})
  ENDFOREACH()
  FOREACH(option ${option_names})
    SET(${prefix}_${option} FALSE)
  ENDFOREACH()

  SET(current_arg_name DEFAULT_ARGS)
  SET(current_arg_list)
  FOREACH(arg ${ARGN})            
    SET(larg_names ${arg_names})    
    LIST(FIND larg_names "${arg}" is_arg_name)                   
    IF (is_arg_name GREATER -1)
      SET(${prefix}_${current_arg_name} ${current_arg_list})
      SET(current_arg_name ${arg})
      SET(current_arg_list)
    ELSE ()
      SET(loption_names ${option_names})    
      LIST(FIND loption_names "${arg}" is_option)            
      IF (is_option GREATER -1)
       SET(${prefix}_${arg} TRUE)
      ELSE ()
       SET(current_arg_list ${current_arg_list} ${arg})
      ENDIF ()
    ENDIF ()
  ENDFOREACH()
  SET(${prefix}_${current_arg_name} ${current_arg_list})
ENDMACRO()

#----------------------------------------------------------------------------
# Macro for extracting Plugin path and name from arguments
#----------------------------------------------------------------------------
MACRO(PV_EXTRACT_CLIENT_SERVER_ARGS)
  set(options)
  set(oneValueArgs LOAD_PLUGIN PLUGIN_PATH)
  set(multiValueArgs )
  cmake_parse_arguments(PV "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  set(CLIENT_SERVER_ARGS)
  if(PV_PLUGIN_PATH)
    set(CLIENT_SERVER_ARGS ${CLIENT_SERVER_ARGS} "--test-plugin-path=${PV_PLUGIN_PATH}")
  endif()
  if(PV_LOAD_PLUGIN)
    set(CLIENT_SERVER_ARGS ${CLIENT_SERVER_ARGS} "--test-plugin=${PV_LOAD_PLUGIN}")
  endif()
ENDMACRO()

#----------------------------------------------------------------------------
# Macro for setting values if a user did not overwrite them
#----------------------------------------------------------------------------
MACRO(pv_set_if_not_set name value)
  IF(NOT DEFINED "${name}")
    SET(${name} "${value}")
  ENDIF()
ENDMACRO()

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
    ELSE ()
      GET_FILENAME_COMPONENT(dir_tmp ${libpath} PATH)
      GET_FILENAME_COMPONENT(name_tmp ${libpath} NAME)
      FILE(GLOB lib_list RELATIVE "${dir_tmp}" "${libpath}*")
    ENDIF ()
    INSTALL(CODE "
          MESSAGE(STATUS \"Installing ${name_tmp}\")
          EXECUTE_PROCESS (WORKING_DIRECTORY ${dir_tmp}
               COMMAND tar c ${lib_list}
               COMMAND tar -xC \${CMAKE_INSTALL_PREFIX}/${dest})
               " COMPONENT ${component})
  ENDIF ()
ENDFUNCTION ()

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
ENDFUNCTION ()

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
      COMMAND kwProcessXML
              ${base_64}
              ${name}
              \"${arg_PREFIX}\" 
              \"${arg_SUFFIX}\"
              \"${arg_SUFFIX}\"
              ${input_files}
      DEPENDS ${arg_FILES}
              kwProcessXML 
     ) 
  endif ()

  if (DEFINED arg_VARIABLE)
    set (${arg_VARIABLE} ${function_names} PARENT_SCOPE)
  endif()
endfunction()


# NOTE: coded-separator lists
#
# Workaround for inability to pass ';'-separated lists via command-line.
# From caller: replace '_'  -> '_u' and ';'  -> '_s'
# On receiver: replace '_s' -> ';'  and '_u' -> '_'


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
  # generate_proxydocumentation.cmake and generate_qhp.cmake have code to convert
  # these strings back to lists.
  set (xmls_string "")
  foreach (xml ${xmls})
    get_filename_component(xml "${xml}" ABSOLUTE)
    set (xmls_string "${xmls_string}${xml};")
  endforeach()
  
  set (gui_xmls_string "")
  foreach (gui_xml ${gui_xmls})
    get_filename_component(gui_xml "${gui_xml}" ABSOLUTE)
    set (gui_xmls_string "${gui_xmls_string}${gui_xml};")
  endforeach()

  # Escape ';' in lists
  string(REPLACE "_" "_u"  xmls_string "${xmls_string}")
  string(REPLACE ";" "_s"  xmls_string "${xmls_string}")
  string(REPLACE "_" "_u"  gui_xmls_string "${gui_xmls_string}")
  string(REPLACE ";" "_s"  gui_xmls_string "${gui_xmls_string}")

  set (all_xmls ${xmls} ${gui_xmls})
  list (GET all_xmls 0 first_xml)
  if (NOT first_xml)
    message(FATAL_ERROR "No xml specified!!!")
  endif()
  
  # extract the name from the first xml file. This is the name for temporary
  # file we use.
  get_filename_component(first_xml "${first_xml}" NAME)

  if (PARAVIEW_QT_VERSION STREQUAL "4")
    set(qt_binary_dir_hints "${QT_BINARY_DIR}")
  else() # Qt5
    # Qt5's CMake config doesn't support QT_BINARY_DIR
    set(qt_binary_dir_hints "${Qt5_DIR}/../../../bin")
  endif()

  find_program(QT_XMLPATTERNS_EXECUTABLE
    xmlpatterns
    HINTS "${qt_binary_dir_hints}"
    DOC "xmlpatterns used to generate html from Proxy documentation.")
  mark_as_advanced(QT_XMLPATTERNS_EXECUTABLE)

  if (NOT EXISTS ${QT_XMLPATTERNS_EXECUTABLE})
    message(WARNING "Valid QT_XMLPATTERNS_EXECUTABLE not specified.")
  else()

    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${first_xml}"

      # process each html file to separate it out into files for each proxy.
      COMMAND ${CMAKE_COMMAND}
              -Dxmlpatterns:FILEPATH=${QT_XMLPATTERNS_EXECUTABLE}
              -Dxml_to_xml_xsl:FILEPATH=${ParaView_CMAKE_DIR}/smxml_to_xml.xsl
              -Dgenerate_category_rw_xsl:FILEPATH=${ParaView_CMAKE_DIR}/generate_category_rw.xsl
              -Dxml_to_html_xsl:FILEPATH=${ParaView_CMAKE_DIR}/xml_to_html.xsl
              -Dxml_to_wiki_xsl:FILEPATH=${ParaView_CMAKE_DIR}/xml_to_wiki.xsl.in
              -Dinput_xmls:STRING=${xmls_string}
              -Dinput_gui_xmls:STRING=${gui_xmls_string}
              -Doutput_dir:PATH=${output_dir}
              -Doutput_file:FILEPATH=${CMAKE_CURRENT_BINARY_DIR}/${first_xml}
              -P ${ParaView_CMAKE_DIR}/generate_proxydocumentation.cmake

      DEPENDS ${xmls}
              ${ParaView_CMAKE_DIR}/smxml_to_xml.xsl
              ${ParaView_CMAKE_DIR}/xml_to_html.xsl
              ${ParaView_CMAKE_DIR}/generate_proxydocumentation.cmake

      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"

      COMMENT "Generating Documentation HTMLs from xmls")

    set (dependencies ${dependencies}
         "${CMAKE_CURRENT_BINARY_DIR}/${first_xml}")
    set (${output_files} ${dependencies} PARENT_SCOPE)
  endif()
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

  if (NOT PARAVIEW_ENABLE_EMBEDDED_DOCUMENTATION)
    return()
  endif()

  if (NOT DEFINED arg_DESTINATION_DIRECTORY)
    message(FATAL_ERROR "No DESTINATION_DIRECTORY specified in build_help_project()")
  endif()

  if (PARAVIEW_QT_VERSION STREQUAL "4")
    set(qt_binary_dir_hints "${QT_BINARY_DIR}")
  else() # Qt5
    # Qt5's CMake config doesn't support QT_BINARY_DIR
    set(qt_binary_dir_hints "${Qt5_DIR}/../../../bin")
  endif()

  find_program(QT_HELP_GENERATOR
    qhelpgenerator
    HINTS "${qt_binary_dir_hints}"
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

  set (copy_directory_command)
  if (arg_DOCUMENTATION_SOURCE_DIR)
    set (copy_directory_command
      # copy all htmls from source to destination directory (same location where the
      # qhp file is present.
      COMMAND ${CMAKE_COMMAND} -E copy_directory
              "${arg_DOCUMENTATION_SOURCE_DIR}"
              "${arg_DESTINATION_DIRECTORY}"
      )
  endif()

  # sanitize arg_FILEPATTERNS since we pass it as a command line argument.
  # Escape ';' in lists
  string(REPLACE "_" "_u"  arg_FILEPATTERNS "${arg_FILEPATTERNS}")
  string(REPLACE ";" "_s"  arg_FILEPATTERNS "${arg_FILEPATTERNS}")

  # Remove newlines from the table of contents.
  string(REPLACE "\n" " " arg_TABLE_OF_CONTENTS "${arg_TABLE_OF_CONTENTS}")

  ADD_CUSTOM_COMMAND(
    OUTPUT ${arg_DESTINATION_DIRECTORY}/${name}.qch
    DEPENDS ${arg_DEPENDS}
            ${ParaView_CMAKE_DIR}/generate_qhp.cmake

    ${copy_directory_command}

    # generate the toc at run-time.
    VERBATIM
    COMMAND ${CMAKE_COMMAND}
            -Doutput_file:FILEPATH=${qhp_filename}
            "-Dfile_patterns:STRING=${arg_FILEPATTERNS}"
            -Dnamespace:STRING=${arg_NAMESPACE}
            -Dfolder:PATH=${arg_FOLDER}
            -Dname:STRING=${name}
            "-Dgiven_toc:STRING=${arg_TABLE_OF_CONTENTS}"
            -P "${ParaView_CMAKE_DIR}/generate_qhp.cmake"

    # Now, compile the qhp file to generate the qch.
    COMMAND ${QT_HELP_GENERATOR}
            ${qhp_filename}
            -o ${arg_DESTINATION_DIRECTORY}/${name}.qch

    COMMENT "Compiling Qt help project ${name}.qhp"

    WORKING_DIRECTORY "${arg_DESTINATION_DIRECTORY}"
  )
endfunction()

macro(pv_set_link_interface_libs target)
  # if not lion then we need to set LINK_INTERFACE_LIBRARIES to reduce the number
  # of libraries we link against there is a limit of 253.
  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND CMAKE_SYSTEM_VERSION VERSION_LESS 11.0) 
    set_property(TARGET ${target}
      PROPERTY LINK_INTERFACE_LIBRARIES "${ARGN}")
  endif()
endmacro()

#------------------------------------------------------------------------------
# replacement for vtk-add executable that also adds the install rules.
#------------------------------------------------------------------------------
include(pvForwardingExecutable)

function(pv_add_executable name)
  set (VTK_EXE_SUFFIX)
  if(UNIX AND VTK_BUILD_FORWARDING_EXECUTABLES)
    set(PV_INSTALL_LIBRARY_DIR ${VTK_INSTALL_LIBRARY_DIR})
    pv_add_executable_with_forwarding(VTK_EXE_SUFFIX ${name} ${ARGN})
    set_property(GLOBAL APPEND PROPERTY VTK_TARGETS ${name})
  else()
    add_executable(${name} ${ARGN})
    set_property(GLOBAL APPEND PROPERTY VTK_TARGETS ${name})
  endif()
  if (PV_EXE_JOB_LINK_POOL)
    set_property(TARGET "${name}" PROPERTY JOB_POOL_LINK ${PV_EXE_JOB_LINK_POOL})
  endif ()
  if (APPLE AND NOT PARAVIEW_DO_UNIX_STYLE_INSTALLS)
    set_target_properties("${name}" PROPERTIES
      INSTALL_RPATH "@executable_path/../Libraries;@executable_path/../Plugins")
  endif ()
  pv_executable_install(${name} "${VTK_EXE_SUFFIX}")
endfunction()

#------------------------------------------------------------------------------
# Function used to add install rules for executables.
#------------------------------------------------------------------------------
function (pv_executable_install name exe_suffix)
  if (NOT VTK_INSTALL_NO_RUNTIME)
    if (exe_suffix)
      # we have two executables to install, one in the bin dir and another in the
      # lib dir

      # install the real-binary in the lib-dir
      install(TARGETS ${name}
              DESTINATION ${VTK_INSTALL_LIBRARY_DIR}
              COMPONENT Runtime)
    endif()

    # install the launcher binary in the binary dir. When exe_suffix is empty, the
    # launcher binary is same as the real binary.
    install(TARGETS ${name}${exe_suffix}
            DESTINATION ${VTK_INSTALL_RUNTIME_DIR}
            COMPONENT Runtime)
  endif()
endfunction()

#------------------------------------------------------------------------------
# Function used to copy arbitrary files matching certain patterns.
# Usage:
# copy_files_recursive(<source-dir>
#   DESTINATION <destination-dir>
#   [LABEL "<label to use>"]
#   [OUTPUT "<file generated to mark end of copying>"]
#   [REGEX <regex> [EXCLUDE]]
#   )
# One can specify multiple REGEX or REGEX <regex> EXCLUDE arguments.
#------------------------------------------------------------------------------
function(copy_files_recursive source-dir)
  set (dest-dir)
  set (patterns)
  set (exclude-patterns)
  set (output-file)
  set (label "Copying files")

  set (doing "")
  foreach (arg ${ARGN})
    if (arg MATCHES "^(DESTINATION|REGEX|OUTPUT|LABEL)$")
      set (doing "${arg}")
    elseif ("${doing}" STREQUAL "DESTINATION")
      set (doing "")
      set (dest-dir "${arg}")
    elseif ("${doing}" STREQUAL "REGEX")
      set (doing "SET")
      list (APPEND patterns "${arg}")
    elseif (("${arg}" STREQUAL "EXCLUDE") AND ("${doing}" STREQUAL "SET"))
      set (doing "")
      list (GET patterns -1 cur-pattern)
      list (REMOVE_AT patterns -1)
      list (APPEND exclude-patterns "${cur-pattern}")
    elseif ("${doing}" STREQUAL "OUTPUT")
      set (doing "")
      set (output-file "${arg}")
    elseif ("${doing}" STREQUAL "LABEL")
      set (doing "")
      set (label "${arg}")
    else()
      message(AUTHOR_WARNING "Unknown argument [${arg}]")
    endif()
  endforeach()

  set (match-regex)
  foreach (_item ${patterns})
    if (match-regex)
      set (match-regex "${match-regex}")
    endif()
    set (match-regex "${match-regex}${_item}")
  endforeach()

  set (exclude-regex)
  foreach (_item ${exclude-patterns})
    if (exclude-regex)
      set (exclude-regex "${exclude-regex}|")
    endif()
    set (exclude-regex "${exclude-regex}${_item}")
  endforeach()

  file(GLOB_RECURSE _all_files RELATIVE "${source-dir}" "${source-dir}/*")
  
  set (all_files)
  set (copy-commands)
  foreach (_file ${_all_files})
    if (exclude-regex AND ("${_file}" MATCHES "${exclude-regex}"))
      # skip
    elseif ("${_file}" MATCHES "${match-regex}")
      set (in-file "${source-dir}/${_file}")
      set (out-file "${dest-dir}/${_file}")
      get_filename_component(out-path ${out-file} PATH)
      list (APPEND all_files ${in-file})
      set (copy-commands "${copy-commands}
        file(COPY \"${in-file}\" DESTINATION \"${out-path}\")")
    endif()
  endforeach()

  get_filename_component(_name ${output-file} NAME)
  set(CMAKE_CONFIGURABLE_FILE_CONTENT ${copy-commands})
  configure_file(${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in
    "${CMAKE_CURRENT_BINARY_DIR}/${_name}.cfr.cmake" @ONLY)
  unset(CMAKE_CONFIGURABLE_FILE_CONTENT)

  add_custom_command(OUTPUT ${output-file}
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/${_name}.cfr.cmake"
    COMMAND ${CMAKE_COMMAND} -E touch ${output-file}
    DEPENDS ${all_files}
            "${CMAKE_CURRENT_BINARY_DIR}/${_name}.cfr.cmake"
    COMMENT ${label})
endfunction()
