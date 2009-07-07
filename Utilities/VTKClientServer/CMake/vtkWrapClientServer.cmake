#
# a cmake command to client-server wrap classes
#

MACRO(VTK_WRAP_ClientServer TARGET SRC_LIST_NAME SOURCES)

  # clear some variables
  SET (CXX_CONTENTS)
  SET (CXX_CONTENTS2)
  SET (CXX_CONTENTS3) 

  # VS 6 does not like needing to run a huge number of custom commands
  # when building a single target.  Generate some extra custom targets
  # that run the custom commands before the main target is built.  This
  # is a hack to work-around the limitation.
  IF(CMAKE_GENERATOR MATCHES "^Visual Studio 6$")
    SET(VTK_WRAP_CS_CUSTOM_TARGETS_LIMIT 128)
    SET(VTK_WRAP_CS_CUSTOM_NAME ${TARGET})
    SET(VTK_WRAP_CS_CUSTOM_LIST)
  ENDIF(CMAKE_GENERATOR MATCHES "^Visual Studio 6$")

  # if this is used from outside paraview (e.g. in a plugin, this
  # should come from the ParaViewConfig.cmake file
  IF(NOT VTK_WRAP_ClientServer_EXE)
    IF(CMAKE_CROSSCOMPILING)
      SET(VTK_WRAP_ClientServer_EXE vtkWrapClientServer)
    ELSE(CMAKE_CROSSCOMPILING)
      GET_TARGET_PROPERTY(VTK_WRAP_ClientServer_EXE vtkWrapClientServer LOCATION)
    ENDIF(CMAKE_CROSSCOMPILING)
  ENDIF(NOT VTK_WRAP_ClientServer_EXE)
  # still not found ?
  IF(NOT VTK_WRAP_ClientServer_EXE)
    MESSAGE(FATAL_ERROR "did not find ClientServer for target: ${TARGET}")
  ENDIF(NOT VTK_WRAP_ClientServer_EXE)

  # For each class
  FOREACH(FILE ${SOURCES})

    # should we wrap the file?
    GET_SOURCE_FILE_PROPERTY(TMP_WRAP_EXCLUDE ${FILE} WRAP_EXCLUDE)
    
    # if we should wrap it
    IF (NOT TMP_WRAP_EXCLUDE)
        
      # what is the filename without the extension
      GET_FILENAME_COMPONENT(TMP_FILENAME ${FILE} NAME_WE)
      
      # the input file might be full path so handle that
      GET_FILENAME_COMPONENT(TMP_FILEPATH ${FILE} PATH)
      
      # compute the input filename
      IF (TMP_FILEPATH)
        SET(TMP_INPUT ${TMP_FILEPATH}/${TMP_FILENAME}.h) 
      ELSE (TMP_FILEPATH)
        SET(TMP_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/${TMP_FILENAME}.h)
      ENDIF (TMP_FILEPATH)
      
      # is it abstract?
      GET_SOURCE_FILE_PROPERTY(TMP_ABSTRACT ${FILE} ABSTRACT)
      IF (TMP_ABSTRACT)
        SET(TMP_CONCRETE 0)
      ELSE (TMP_ABSTRACT)
        SET(TMP_CONCRETE 1)
        # add it to the init file's contents
        SET (CXX_CONTENTS 
          "${CXX_CONTENTS}int ${TMP_FILENAME}Command(vtkClientServerInterpreter *, vtkObjectBase *, const char *, const vtkClientServerStream&, vtkClientServerStream& resultStrem);\nvtkObjectBase *${TMP_FILENAME}ClientServerNewCommand();\n")
        
        SET (CXX_CONTENTS2 
          "${CXX_CONTENTS2}  arlu->AddCommandFunction(\"${TMP_FILENAME}\",${TMP_FILENAME}Command);\n")
        
        SET (CXX_CONTENTS3 
          "${CXX_CONTENTS3}    if (!strcmp(\"${TMP_FILENAME}\",type))\n      {\n      vtkObjectBase *ptr = ${TMP_FILENAME}ClientServerNewCommand();\n      arlu->NewInstance(ptr,id);\n      return 1;\n      }\n")
      ENDIF (TMP_ABSTRACT)
      
      # new source file is nameClientServer.cxx, add to resulting list
      SET(${SRC_LIST_NAME} ${${SRC_LIST_NAME}} 
        ${TMP_FILENAME}ClientServer.cxx)

      # add custom command to output
      ADD_CUSTOM_COMMAND(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TMP_FILENAME}ClientServer.cxx
        MAIN_DEPENDENCY ${TMP_INPUT}
        DEPENDS ${VTK_WRAP_ClientServer_EXE} ${VTK_WRAP_HINTS}
        COMMAND ${VTK_WRAP_ClientServer_EXE}
        ARGS ${TMP_INPUT} ${VTK_WRAP_HINTS} ${TMP_CONCRETE} 
        ${CMAKE_CURRENT_BINARY_DIR}/${TMP_FILENAME}ClientServer.cxx
        )

      # Add this output to a custom target if needed.
      IF(VTK_WRAP_CS_CUSTOM_TARGETS_LIMIT)
        LIST(APPEND VTK_WRAP_CS_CUSTOM_LIST ${CMAKE_CURRENT_BINARY_DIR}/${TMP_FILENAME}ClientServer.cxx)
        LIST(LENGTH VTK_WRAP_CS_CUSTOM_LIST VTK_WRAP_CS_CUSTOM_COUNT)
        IF("${VTK_WRAP_CS_CUSTOM_COUNT}" EQUAL "${VTK_WRAP_CS_CUSTOM_TARGETS_LIMIT}")
          SET(VTK_WRAP_CS_CUSTOM_NAME ${VTK_WRAP_CS_CUSTOM_NAME}Hack)
          ADD_CUSTOM_TARGET(${VTK_WRAP_CS_CUSTOM_NAME} DEPENDS ${VTK_WRAP_CS_CUSTOM_LIST})
          SET(KIT_CS_DEPS ${VTK_WRAP_CS_CUSTOM_NAME})
          SET(VTK_WRAP_CS_CUSTOM_LIST)
        ENDIF("${VTK_WRAP_CS_CUSTOM_COUNT}" EQUAL "${VTK_WRAP_CS_CUSTOM_TARGETS_LIMIT}")
      ENDIF(VTK_WRAP_CS_CUSTOM_TARGETS_LIMIT)
    ENDIF (NOT TMP_WRAP_EXCLUDE)
  ENDFOREACH(FILE)
  
  # need the set for the configure file
  SET (CS_TARGET ${TARGET})

  CONFIGURE_FILE(
    ${VTKCS_CONFIG_DIR}/vtkWrapClientServer.cxx.in  
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.cxx
    COPY_ONLY
    IMMEDIATE
    )

  # Create the Init File
  SET(${SRC_LIST_NAME} ${${SRC_LIST_NAME}} 
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.cxx)
  SET_SOURCE_FILES_PROPERTIES(
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.cxx 
    PROPERTIES GENERATED 1 WRAP_EXCLUDE 1 ABSTRACT 0
    )
  
ENDMACRO(VTK_WRAP_ClientServer)

