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

  # all the compiler "-D" args
  get_directory_property(TMP_DEF_LIST DEFINITION COMPILE_DEFINITIONS)
  set(TMP_DEFINITIONS)
  foreach(TMP_DEF ${TMP_DEF_LIST})
    set(TMP_DEFINITIONS ${TMP_DEFINITIONS} -D "${TMP_DEF}")
  endforeach()

  # hints that guide wrapping
  if (VTK_WRAP_HINTS)
    set(TMP_HINTS "--hints" "${VTK_WRAP_HINTS}")
  else ()
    set(TMP_HINTS)
  endif ()

  # take all the include directories of this module into account
  set(TMP_INCLUDE_DIRS ${VTK_INCLUDE_DIR})
  set(TMP_INCLUDE)
  foreach(INCLUDE_DIR ${TMP_INCLUDE_DIRS})
    set(TMP_INCLUDE ${TMP_INCLUDE} -I "${INCLUDE_DIR}")
  endforeach()

  set(_include_dirs_file)
  foreach(INCLUDE_DIR ${TMP_INCLUDE_DIRS})
    set(_include_dirs_file "${_include_dirs_file}${INCLUDE_DIR}\n")
  endforeach()

  string(STRIP "${_include_dirs_file}" CMAKE_CONFIGURABLE_FILE_CONTENT)
  set(_target_includes_file ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.inc)
  configure_file(${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in
                 ${_target_includes_file} @ONLY)
  set(_target_includes "--includes ${_target_includes_file}")

  # For each class
  foreach(FILE ${SOURCES})

    # should we wrap the file?
    get_source_file_property(TMP_WRAP_EXCLUDE ${FILE} WRAP_EXCLUDE)

    # if we should wrap it
    if (NOT TMP_WRAP_EXCLUDE)

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

      # is it abstract?
      get_source_file_property(TMP_ABSTRACT ${FILE} ABSTRACT)
      if (TMP_ABSTRACT)
        set(TMP_CONCRETE "--abstract")
      else ()
        set(TMP_CONCRETE "--concrete")
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
        DEPENDS ${VTK_WRAP_ClientServer_EXE} ${VTK_WRAP_HINTS} ${_target_includes_file}
        COMMAND ${VTK_WRAP_ClientServer_EXE}
        ARGS
        ${TMP_CONCRETE}
        ${TMP_HINTS}
        ${TMP_DEFINITIONS}
        ${_target_includes}
        "-o" "${quote}${CMAKE_CURRENT_BINARY_DIR}/${TMP_FILENAME}ClientServer.cxx${quote}"
        "${quote}${TMP_INPUT}${quote}"
        COMMENT "CS Wrapping - generating ${TMP_FILENAME}ClientServer.cxx"
        )

    endif (NOT TMP_WRAP_EXCLUDE)
  endforeach(FILE)

  # Create the Init File
  set (CS_TARGET ${TARGET})
  configure_file(
    ${ParaView_CMAKE_DIR}/vtkWrapClientServer.cxx.in
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.cxx
    COPY_ONLY
    IMMEDIATE
    )
  #add it to the list of files to compile for the CS wrapped lib
  set(${SRC_LIST_NAME} ${${SRC_LIST_NAME}}
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.cxx)
  set_source_files_properties(
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.cxx
    PROPERTIES GENERATED 1 WRAP_EXCLUDE 1 ABSTRACT 0
    )

endmacro(VTK_WRAP_ClientServer)
