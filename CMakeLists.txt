PROJECT(ParaViewComplete)
SUBDIRS(VTK ParaView)
SUBDIR_DEPENDS(ParaView VTK)

SET (LIBRARY_OUTPUT_PATH ${ParaViewComplete_BINARY_DIR}/bin/ CACHE PATH 
"Single output directory for building all libraries.")
SET (EXECUTABLE_OUTPUT_PATH ${ParaViewComplete_BINARY_DIR}/bin/ CACHE PATH 
"Single output directory for building all executables.")

# force these to be ON by default as they are required by ParaView
OPTION (VTK_USE_HYBRID "Build the hybrid directory classes" ON)
OPTION (VTK_USE_PARALLEL "Build the parallel directory classes" ON)
OPTION (VTK_WRAP_TCL "wrap classes into the TCL intepreted language" ON)

IF(UNIX)
  IF(RUN_CONFIGURE)
    EXEC_PROGRAM("${ParaViewComplete_SOURCE_DIR}/tcl8.2.3/unix/configure --disable-shared"
               ${ParaViewComplete_BINARY_DIR}/tcl8.2.3/unix)
    EXEC_PROGRAM("${ParaViewComplete_SOURCE_DIR}/tk8.2.3/unix/configure --disable-shared"
               ${ParaViewComplete_BINARY_DIR}/tk8.2.3/unix)
  ENDIF(RUN_CONFIGURE)
  SET(TCL_LIBRARY ${ParaViewComplete_BINARY_DIR}/tcl8.2.3/unix/libtcl8.2.a CACHE FILEPATH
   "Path to tcl library")
  SET(TCL_INCLUDE_PATH ${ParaViewComplete_SOURCE_DIR}/tcl8.2.3/generic CACHE PATH "Path to tcl include")
  SET(TK_LIBRARY ${ParaViewComplete_BINARY_DIR}/tk8.2.3/unix/libtk8.2.a CACHE FILEPATH
   "Path to tk library")
  SET(TK_INCLUDE_PATH ${ParaViewComplete_SOURCE_DIR}/tk8.2.3/generic CACHE PATH "Path to tk include")
ENDIF(UNIX)


ADD_CUSTOM_TARGET(maketcl ALL "cd ${ParaViewComplete_BINARY_DIR}/tcl8.2.3/unix; make" )
ADD_CUSTOM_TARGET(maketk ALL "cd ${ParaViewComplete_BINARY_DIR}/tk8.2.3/unix; make" )
