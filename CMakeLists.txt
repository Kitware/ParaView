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
    CONFIGURE_FILE(
     ${ParaViewComplete_SOURCE_DIR}/TclConfig.txt.in
     ${ParaViewComplete_BINARY_DIR}/tcl8.2.3/unix/TclConfig.txt)

    CONFIGURE_FILE(
     ${ParaViewComplete_SOURCE_DIR}/TclConfig.txt.in
     ${ParaViewComplete_BINARY_DIR}/tk8.2.3/unix/TclConfig.txt)

    EXEC_PROGRAM("${ParaViewComplete_SOURCE_DIR}/tcl8.2.3/unix/configure --disable-shared --cache-file=TclConfig.txt"
               ${ParaViewComplete_BINARY_DIR}/tcl8.2.3/unix)
    EXEC_PROGRAM("${ParaViewComplete_SOURCE_DIR}/tk8.2.3/unix/configure --disable-shared --cache-file=TclConfig.txt"
               ${ParaViewComplete_BINARY_DIR}/tk8.2.3/unix)
  ENDIF(RUN_CONFIGURE)
  SET(TCL_LIBRARY ${ParaViewComplete_BINARY_DIR}/tcl8.2.3/unix/libtcl8.2.a CACHE FILEPATH
   "Path to tcl library")
  SET(TCL_INCLUDE_PATH ${ParaViewComplete_SOURCE_DIR}/tcl8.2.3/generic CACHE PATH "Path to tcl include")
  SET(TK_LIBRARY ${ParaViewComplete_BINARY_DIR}/tk8.2.3/unix/libtk8.2.a CACHE FILEPATH
   "Path to tk library")
  SET(TK_INCLUDE_PATH ${ParaViewComplete_SOURCE_DIR}/tk8.2.3/generic CACHE PATH "Path to tk include")

  ADD_CUSTOM_TARGET(maketcl ALL cd ${ParaViewComplete_BINARY_DIR}/tcl8.2.3/unix; make )
  ADD_CUSTOM_TARGET(maketk ALL cd ${ParaViewComplete_BINARY_DIR}/tk8.2.3/unix; make )
ELSE(UNIX)
  SET(TCL_LIBRARY ${ParaViewComplete_SOURCE_DIR}/TclTk8.2.3Win/lib/tcl82.lib CACHE FILEPATH
   "Path to tcl library")
  SET(TCL_INCLUDE_PATH ${ParaViewComplete_SOURCE_DIR}/TclTk8.2.3Win/include CACHE PATH "Path to tcl include")
  SET(TK_LIBRARY ${ParaViewComplete_SOURCE_DIR}/TclTk8.2.3Win/lib/tk82.lib CACHE FILEPATH
   "Path to tcl library")
  SET(TK_INCLUDE_PATH ${ParaViewComplete_SOURCE_DIR}/TclTk8.2.3Win/include CACHE PATH "Path to tcl include")
  SET(TK_INTERNAL_PATH ${ParaViewComplete_SOURCE_DIR}/VTK/Rendering/tkInternals CACHE PATH "Path to tcl include")
# copy the tcl/tk dll's to Release Debug MinSizeRel and RelWithDebInfo bin directory
# so no paths are needed to run this.
  CONFIGURE_FILE(${ParaViewComplete_SOURCE_DIR}/TclTk8.2.3Win/bin/tcl82.dll
                 ${ParaViewComplete_BINARY_DIR}/bin/Debug/tcl82.dll COPYONLY)
  CONFIGURE_FILE(${ParaViewComplete_SOURCE_DIR}/TclTk8.2.3Win/bin/tcl82.dll
                 ${ParaViewComplete_BINARY_DIR}/bin/Release/tcl82.dll COPYONLY)
  CONFIGURE_FILE(${ParaViewComplete_SOURCE_DIR}/TclTk8.2.3Win/bin/tcl82.dll
                 ${ParaViewComplete_BINARY_DIR}/bin/MinSizeRel/tcl82.dll COPYONLY)
  CONFIGURE_FILE(${ParaViewComplete_SOURCE_DIR}/TclTk8.2.3Win/bin/tcl82.dll
                 ${ParaViewComplete_BINARY_DIR}/bin/RelWithDebInfo/tcl82.dll COPYONLY)
  CONFIGURE_FILE(${ParaViewComplete_SOURCE_DIR}/TclTk8.2.3Win/bin/tk82.dll
                 ${ParaViewComplete_BINARY_DIR}/bin/Debug/tk82.dll COPYONLY)
  CONFIGURE_FILE(${ParaViewComplete_SOURCE_DIR}/TclTk8.2.3Win/bin/tk82.dll
                 ${ParaViewComplete_BINARY_DIR}/bin/Release/tk82.dll COPYONLY)
  CONFIGURE_FILE(${ParaViewComplete_SOURCE_DIR}/TclTk8.2.3Win/bin/tk82.dll
                 ${ParaViewComplete_BINARY_DIR}/bin/MinSizeRel/tk82.dll COPYONLY)
  CONFIGURE_FILE(${ParaViewComplete_SOURCE_DIR}/TclTk8.2.3Win/bin/tk82.dll
                 ${ParaViewComplete_BINARY_DIR}/bin/RelWithDebInfo/tk82.dll COPYONLY)
ENDIF(UNIX)


