#
# a cmake command to client-server wrap classes
#

macro(VTK_WRAP_ClientServer TARGET SRC_LIST_NAME SOURCES)

  # clear some variables
  set (CXX_CONTENTS)
  set (CXX_CONTENTS2)

  # if this is used from outside paraview (e.g. in a plugin, this
  # should come from the ParaViewConfig.cmake file
  if(NOT VTK_WRAP_ClientServer_EXE)
    if (TARGET vtkWrapClientServer)
      set(VTK_WRAP_ClientServer_EXE vtkWrapClientServer)
    else ()
      message(FATAL_ERROR "VTK_WRAP_ClientServer_EXE must be set.")
    endif()
  endif()

  # all the include directories
  if(VTK_WRAP_INCLUDE_DIRS)
    set(TMP_INCLUDE_DIRS ${VTK_WRAP_INCLUDE_DIRS})
  else()
    set(TMP_INCLUDE_DIRS ${VTK_INCLUDE_DIRS})
  endif()

  # collect the common wrapper-tool arguments
  set(_common_args)
  get_directory_property(_def_list DEFINITION COMPILE_DEFINITIONS)
  foreach(TMP_DEF ${_def_list})
    set(_common_args "${_common_args}-D${TMP_DEF}\n")
  endforeach()
  foreach(INCLUDE_DIR ${TMP_INCLUDE_DIRS})
    set(_common_args "${_common_args}-I\"${INCLUDE_DIR}\"\n")
  endforeach()
  if(VTK_WRAP_HINTS)
    set(_common_args "${_common_args}--hints \"${VTK_WRAP_HINTS}\"\n")
  endif()
  if(KIT_HIERARCHY_FILE)
    set(_common_args "${_common_args}--types \"${KIT_HIERARCHY_FILE}\"\n")
  endif()

  # write wrapper-tool arguments to a file
  string(STRIP "${_common_args}" CMAKE_CONFIGURABLE_FILE_CONTENT)
  set(_args_file ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.args)
  configure_file(${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in
                 ${_args_file} @ONLY)

  set (CS_TARGET ${TARGET})
  string(REGEX REPLACE "CS$" "" BARE_TARGET "${CS_TARGET}")

  if (NOT TARGET ${BARE_TARGET}PythonD)
    set(NO_PYTHON_BINDINGS_AVAILABLE TRUE)
  endif ()

  # For each class
  foreach(FILE ${SOURCES})

    # should we wrap the file?
    get_source_file_property(TMP_WRAP_EXCLUDE ${FILE} WRAP_EXCLUDE)

    # if we should wrap it
    if (NOT TMP_WRAP_EXCLUDE AND
        (NOT PARAVIEW_USE_UNIFIED_BINDINGS OR
         NO_PYTHON_BINDINGS_AVAILABLE))

      # what is the filename without the extension
      get_filename_component(TMP_FILENAME ${FILE} NAME_WE)

      # the input file might be full path so handle that
      get_filename_component(TMP_FILEPATH ${FILE} PATH)

      # compute the input filename
      if (TMP_FILEPATH)
        set(TMP_INPUT ${TMP_FILEPATH}/${TMP_FILENAME}.h)
      else ()
        set(TMP_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/${TMP_FILENAME}.h)
      endif ()

      # add it to the init file's contents
      set (CXX_CONTENTS
        "${CXX_CONTENTS}extern void ${TMP_FILENAME}_Init(vtkClientServerInterpreter* csi);\n")

      set (CXX_CONTENTS2
        "${CXX_CONTENTS2}  ${TMP_FILENAME}_Init(csi);\n")

      # new source file is nameClientServer.cxx, add to resulting list of cs wrapped files to compile
      set(${SRC_LIST_NAME} ${${SRC_LIST_NAME}}
        ${TMP_FILENAME}ClientServer.cxx)

      # add custom command to generate the cs wrapped file
      add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TMP_FILENAME}ClientServer.cxx
        MAIN_DEPENDENCY ${TMP_INPUT}
        DEPENDS ${VTK_WRAP_ClientServer_EXE} ${VTK_WRAP_HINTS} ${_target_includes_file} ${_args_file}
        COMMAND ${VTK_WRAP_ClientServer_EXE}
        ARGS
        ${TMP_HINTS}
        "${quote}@${_args_file}${quote}"
        "-o" "${quote}${CMAKE_CURRENT_BINARY_DIR}/${TMP_FILENAME}ClientServer.cxx${quote}"
        "${quote}${TMP_INPUT}${quote}"
        COMMENT "CS Wrapping - generating ${TMP_FILENAME}ClientServer.cxx"
        )

    endif ()
  endforeach()

  # Create the Init File
  configure_file(
    ${ParaView_CMAKE_DIR}/vtkWrapClientServer.cxx.in
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.cxx
    @ONLY
    IMMEDIATE
    )
  #add it to the list of files to compile for the CS wrapped lib
  set(${SRC_LIST_NAME} ${${SRC_LIST_NAME}}
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.cxx)
  set_source_files_properties(
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.cxx
    PROPERTIES GENERATED 1 WRAP_EXCLUDE 1 ABSTRACT 0
    )

  unset(NO_PYTHON_BINDINGS_AVAILABLE)

endmacro()
