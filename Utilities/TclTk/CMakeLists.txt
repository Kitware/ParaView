PROJECT(TclTk)

IF(UNIX)
  SET(TCL_TK_STATIC 1 CACHE INTERNAL "Build with static Tcl/Tk support. TCL_LIBRARY and TK_LIBRARY must point to the corresponding Tcl/Tk static libraries (example, tcl84sx.lib, tk84sx.lib)." FORCE)
  SET(TclTk_BINARY_DIR_TCLBIN "${TclTk_BINARY_DIR}/tcl8.4.5/unix")
  SET(TclTk_BINARY_DIR_TKBIN "${TclTk_BINARY_DIR}/tk8.4.5/unix")

  FOREACH(tcltkdir "${TclTk_BINARY_DIR_TCLBIN}" "${TclTk_BINARY_DIR_TKBIN}")
    IF(EXISTS ${tcltkdir})
    ELSE(EXISTS ${tcltkdir})
      MAKE_DIRECTORY(${tcltkdir})
    ENDIF(EXISTS ${tcltkdir})
  ENDFOREACH(tcltkdir)

  SET (TCL_TCLSH ${TclTk_BINARY_DIR_TCLBIN}/tclsh 
    CACHE INTERNAL "tclshell exec" FORCE)
  SET(TCL_LIBRARY ${TclTk_BINARY_DIR_TCLBIN}/libtcl8.4.a 
    CACHE INTERNAL "Path to tcl library" FORCE)
  SET(TCL_STUB_LIBRARY 
    ${TclTk_BINARY_DIR_TCLBIN}/libtclstub8.4.a 
    CACHE INTERNAL "Path to tcl stub library" FORCE)
  SET(TCL_INCLUDE_PATH ${TclTk_SOURCE_DIR}/tcl8.4.5/generic 
    CACHE INTERNAL "Path to tcl include" FORCE)
  SET(TK_LIBRARY ${TclTk_BINARY_DIR_TKBIN}/libtk8.4.a 
    CACHE INTERNAL "Path to tk library" FORCE)
  SET(TK_STUB_LIBRARY 
    ${TclTk_BINARY_DIR_TKBIN}/libtkstub8.4.a 
    CACHE INTERNAL "Path to tk stub library" FORCE)
  SET(TK_INCLUDE_PATH ${TclTk_SOURCE_DIR}/tk8.4.5/generic 
    CACHE INTERNAL "Path to tk include" FORCE)

  CONFIGURE_FILE(${TclTk_SOURCE_DIR}/configure_tcltk.sh.in
    ${TclTk_BINARY_DIR}/configure_tcltk.sh IMMEDIATE @ONLY)
  CONFIGURE_FILE(${TclTk_SOURCE_DIR}/build_tcltk.sh.in
    ${TclTk_BINARY_DIR}/build_tcltk.sh IMMEDIATE @ONLY)

  IF(RUN_CONFIGURE)
    EXEC_PROGRAM( "/bin/sh \"${TclTk_BINARY_DIR}/configure_tcltk.sh\"")
  ENDIF(RUN_CONFIGURE)
  ADD_CUSTOM_TARGET(maketcltk ALL "/bin/sh" "${TclTk_BINARY_DIR}/build_tcltk.sh")
ELSE(UNIX)
  SET(PV_TCL_DIR ${TclTk_SOURCE_DIR}/TclTk8.4.5Win)
  SET(TCL_LIBRARY ${PV_TCL_DIR}/lib/tcl84.lib CACHE INTERNAL
    "Path to tcl library" FORCE)
  SET(TCL_LIBRARY_DEBUG ${PV_TCL_DIR}/lib/tcl84.lib CACHE INTERNAL
    "Path to tcl debug library" FORCE)
  SET(TK_LIBRARY ${PV_TCL_DIR}/lib/tk84.lib CACHE INTERNAL
    "Path to tk library" FORCE)
  SET(TK_LIBRARY_DEBUG ${PV_TCL_DIR}/lib/tk84.lib CACHE INTERNAL
    "Path to tk debug library" FORCE)
  SET(TCL_INCLUDE_PATH ${PV_TCL_DIR}/include CACHE INTERNAL
    "Path to tcl include" FORCE)
  SET(TK_INCLUDE_PATH ${PV_TCL_DIR}/include CACHE INTERNAL
    "Path to tk include" FORCE)
  SET(TCL_TK_COPY_SUPPORT_LIBRARY 1 CACHE INTERNAL "Very few users should worry about this option. If VTK is built against a static Tcl/Tk lib (see TCL_TK_STATIC) or a shared Tcl/Tk bundled inside a project with no library support files (ex: ParaView), this variable should be set to ON and both TCL_SUPPORT_LIBRARY_PATH and TK_SUPPORT_LIBRARY_PATH should point to the directories that hold those files (typically, lib/tcl8.4 and lib/tk8.4 for a typical Tcl/Tk installation, or tcl8.4.5/library and tk8.4.5/library for a Tcl/Tk source repository). Once this variable is set to ON, support files will automatically be copied to the build directory and the executables will try to use that location to initialize Tcl/Tk." FORCE)
  SET(TCL_SUPPORT_LIBRARY_PATH ${TclTk_SOURCE_DIR}/tcl8.4.5/library CACHE INTERNAL "Path to the Tcl support library files. See TCL_TK_COPY_SUPPORT_LIBRARY help." FORCE)
  SET(TK_SUPPORT_LIBRARY_PATH ${TclTk_SOURCE_DIR}/tk8.4.5/library CACHE INTERNAL "Path to the Tcl support library files. See TCL_TK_COPY_SUPPORT_LIBRARY help." FORCE)
  # copy the tcl/tk dll's to Release Debug MinSizeRel and RelWithDebInfo bin 
  # directory so no paths are needed to run this.
  IF (CMAKE_CONFIGURATION_TYPES)
    SET (CONFIG_TYPES ${CMAKE_CONFIGURATION_TYPES})
  ELSE (CMAKE_CONFIGURATION_TYPES)
    SET (CONFIG_TYPES .)
  ENDIF (CMAKE_CONFIGURATION_TYPES)

  FOREACH (config ${CONFIG_TYPES})
    CONFIGURE_FILE(${TclTk_SOURCE_DIR}/TclTk8.4.5Win/bin/tcl84.dll
      ${ParaView_BINARY_DIR}/bin/${config}/tcl84.dll 
      COPYONLY)
    CONFIGURE_FILE(${TclTk_SOURCE_DIR}/TclTk8.4.5Win/bin/tk84.dll
      ${ParaView_BINARY_DIR}/bin/${config}/tk84.dll
      COPYONLY)
  ENDFOREACH (config)
ENDIF(UNIX)

