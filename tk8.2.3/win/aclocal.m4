#------------------------------------------------------------------------
# SC_PATH_TCLCONFIG --
#
#	Locate the tclConfig.sh file and perform a sanity check on
#	the Tcl compile flags
#	Currently a no-op for Windows
#
# Arguments:
#	PATCH_LEVEL	The patch level for Tcl if any.
#
# Results:
#
#	Adds the following arguments to configure:
#		--with-tcl=...
#
#	Sets the following vars:
#		TCL_BIN_DIR	Full path to the tclConfig.sh file
#------------------------------------------------------------------------

AC_DEFUN(SC_PATH_TCLCONFIG, [
    AC_MSG_CHECKING([the location of tclConfig.sh])

    if test -d ../../tcl8.2$1/win;  then
	TCL_BIN_DIR_DEFAULT=../../tcl8.2$1/win
    else
	TCL_BIN_DIR_DEFAULT=../../tcl8.2/win
    fi
    
    AC_ARG_WITH(tcl, [  --with-tcl=DIR          use Tcl 8.2 binaries from DIR],
	    TCL_BIN_DIR=$withval, TCL_BIN_DIR=`cd $TCL_BIN_DIR_DEFAULT; pwd`)
    if test ! -d $TCL_BIN_DIR; then
	AC_MSG_ERROR(Tcl directory $TCL_BIN_DIR does not exist)
    fi
    if test ! -f $TCL_BIN_DIR/tclConfig.sh; then
	AC_MSG_ERROR(There is no tclConfig.sh in $TCL_BIN_DIR:  perhaps you did not specify the Tcl *build* directory (not the toplevel Tcl directory) or you forgot to configure Tcl?)
    fi
    AC_MSG_RESULT($TCL_BIN_DIR/tclConfig.sh)
])

#------------------------------------------------------------------------
# SC_PATH_TKCONFIG --
#
#	Locate the tkConfig.sh file
#	Currently a no-op for Windows
#
# Arguments:
#	none
#
# Results:
#
#	Adds the following arguments to configure:
#		--with-tk=...
#
#	Sets the following vars:
#		TK_BIN_DIR	Full path to the tkConfig.sh file
#------------------------------------------------------------------------

AC_DEFUN(SC_PATH_TKCONFIG, [
    AC_MSG_CHECKING([the location of tkConfig.sh])

    if test -d ../../tk8.2$1/win;  then
	TK_BIN_DIR_DEFAULT=../../tk8.2$1/win
    else
	TK_BIN_DIR_DEFAULT=../../tk8.2/win
    fi
    
    AC_ARG_WITH(tk, [  --with-tk=DIR          use Tk 8.2 binaries from DIR],
	    TK_BIN_DIR=$withval, TK_BIN_DIR=`cd $TK_BIN_DIR_DEFAULT; pwd`)
    if test ! -d $TK_BIN_DIR; then
	AC_MSG_ERROR(Tk directory $TK_BIN_DIR does not exist)
    fi
    if test ! -f $TK_BIN_DIR/tkConfig.sh; then
	AC_MSG_ERROR(There is no tkConfig.sh in $TK_BIN_DIR:  perhaps you did not specify the Tk *build* directory (not the toplevel Tk directory) or you forgot to configure Tk?)
    fi

    AC_MSG_RESULT($TK_BIN_DIR/tkConfig.sh)
])

#------------------------------------------------------------------------
# SC_LOAD_TCLCONFIG --
#
#	Load the tclConfig.sh file
#	Currently a no-op for Windows
#
# Arguments:
#	
#	Requires the following vars to be set:
#		TCL_BIN_DIR
#
# Results:
#
#	Subst the following vars:
#		TCL_BIN_DIR
#		TCL_SRC_DIR
#		TCL_LIB_FILE
#
#------------------------------------------------------------------------

AC_DEFUN(SC_LOAD_TCLCONFIG, [
    AC_MSG_CHECKING([for existence of $TCL_BIN_DIR/tclConfig.sh])

    if test -f "$TCL_BIN_DIR/tclConfig.sh" ; then
        AC_MSG_RESULT([loading])
	. $TCL_BIN_DIR/tclConfig.sh
    else
        AC_MSG_RESULT([file not found])
    fi

    # The eval is required to do the TCL_DBGX substitution in the
    # TCL_LIB_FILE variable.

    eval TCL_LIB_FILE=${TCL_LIB_FILE}
    eval TCL_LIB_FLAG=${TCL_LIB_FLAG}

    AC_SUBST(TCL_BIN_DIR)
    AC_SUBST(TCL_SRC_DIR)
    AC_SUBST(TCL_LIB_FILE)
])

#------------------------------------------------------------------------
# SC_LOAD_TKCONFIG --
#
#	Load the tkConfig.sh file
#	Currently a no-op for Windows
#
# Arguments:
#	
#	Requires the following vars to be set:
#		TK_BIN_DIR
#
# Results:
#
#	Sets the following vars that should be in tkConfig.sh:
#		TK_BIN_DIR
#------------------------------------------------------------------------

AC_DEFUN(SC_LOAD_TKCONFIG, [
    AC_MSG_CHECKING([for existence of $TCLCONFIG])

    if test -f "$TK_BIN_DIR/tkConfig.sh" ; then
        AC_MSG_CHECKING([loading $TK_BIN_DIR/tkConfig.sh])
	. $TK_BIN_DIR/tkConfig.sh
    else
        AC_MSG_RESULT([could not find $TK_BIN_DIR/tkConfig.sh])
    fi


    AC_SUBST(TK_BIN_DIR)
    AC_SUBST(TK_SRC_DIR)
    AC_SUBST(TK_LIB_FILE)
])

#------------------------------------------------------------------------
# SC_ENABLE_GCC --
#
#	Allows the use of GCC if available
#
# Arguments:
#	none
#	
# Results:
#
#	Adds the following arguments to configure:
#		--enable-gcc
#
#	Sets the following vars:
#		CC	Command to use for the compiler
#------------------------------------------------------------------------

AC_DEFUN(SC_ENABLE_GCC, [
    AC_ARG_ENABLE(gcc, [  --enable-gcc            allow use of gcc if available [--disable-gcc]],
	[ok=$enableval], [ok=no])
    if test "$ok" = "yes"; then
	CC=gcc
	AC_PROG_CC
    else
	CC=cl
    fi
])

#------------------------------------------------------------------------
# SC_ENABLE_SHARED --
#
#	Allows the building of shared libraries
#
# Arguments:
#	none
#	
# Results:
#
#	Adds the following arguments to configure:
#		--enable-shared=yes|no
#
#	Defines the following vars:
#		STATIC_BUILD	Used for building import/export libraries
#				on Windows.
#
#	Sets the following vars:
#		SHARED_BUILD	Value of 1 or 0
#------------------------------------------------------------------------

AC_DEFUN(SC_ENABLE_SHARED, [
    AC_MSG_CHECKING([how to build libraries])
    AC_ARG_ENABLE(shared,
	[  --enable-shared         build and link with shared libraries [--enable-shared]],
    [tcl_ok=$enableval], [tcl_ok=yes])

    if test "${enable_shared+set}" = set; then
	enableval="$enable_shared"
	tcl_ok=$enableval
    else
	tcl_ok=yes
    fi

    if test "$tcl_ok" = "yes" ; then
	AC_MSG_RESULT([shared])
	SHARED_BUILD=1
    else
	AC_MSG_RESULT([static])
	SHARED_BUILD=0
	AC_DEFINE(STATIC_BUILD)
    fi
])

#------------------------------------------------------------------------
# SC_ENABLE_THREADS --
#
#	Specify if thread support should be enabled
#
# Arguments:
#	none
#	
# Results:
#
#	Adds the following arguments to configure:
#		--enable-threads=yes|no
#
#	Defines the following vars:
#		TCL_THREADS
#------------------------------------------------------------------------

AC_DEFUN(SC_ENABLE_THREADS, [
    AC_MSG_CHECKING(for building with threads)
    AC_ARG_ENABLE(threads, [  --enable-threads        build with threads],
	[tcl_ok=$enableval], [tcl_ok=no])

    if test "$tcl_ok" = "yes"; then
	AC_MSG_RESULT(yes)
	TCL_THREADS=1
	AC_DEFINE(TCL_THREADS)
    else
	TCL_THREADS=0
	AC_MSG_RESULT(no (default))
    fi
])

#------------------------------------------------------------------------
# SC_ENABLE_SYMBOLS --
#
#	Specify if debugging symbols should be used
#
# Arguments:
#	none
#	
#	Requires the following vars to be set:
#		CFLAGS_DEBUG
#		CFLAGS_OPTIMIZE
#	
# Results:
#
#	Adds the following arguments to configure:
#		--enable-symbols
#
#	Defines the following vars:
#		CFLAGS_DEFAULT	Sets to CFLAGS_DEBUG if true
#				Sets to CFLAGS_OPTIMIZE if false
#		LDFLAGS_DEFAULT	Sets to LDFLAGS_DEBUG if true
#				Sets to LDFLAGS_OPTIMIZE if false
#		DBGX		Debug library extension
#
#------------------------------------------------------------------------

AC_DEFUN(SC_ENABLE_SYMBOLS, [
    AC_MSG_CHECKING([for build with symbols])
    AC_ARG_ENABLE(symbols, [  --enable-symbols        build with debugging symbols [--disable-symbols]],    [tcl_ok=$enableval], [tcl_ok=no])

    if test "$tcl_ok" = "yes"; then
	CFLAGS_DEFAULT="${CFLAGS_DEBUG}"
	LDFLAGS_DEFAULT="${LDFLAGS_DEBUG}"
	DBGX=d
	AC_MSG_RESULT([yes])
    else
	CFLAGS_DEFAULT="${CFLAGS_OPTIMIZE}"
	LDFLAGS_DEFAULT="${LDFLAGS_OPTIMIZE}"
	DBGX=""
	AC_MSG_RESULT([no])
    fi
])


#--------------------------------------------------------------------
# SC_CONFIG_CFLAGS
#
#	Try to determine the proper flags to pass to the compiler
#	for building shared libraries and other such nonsense.
#
#	NOTE: The backslashes in quotes below are substituted twice
#	due to the fact that they are in a macro and then inlined
#	in the final configure script.
#
# Arguments:
#	none
#
# Results:
#
#	Defines the following vars for all compilers:
#    		EXTRA_CFLAGS
#    		CFLAGS_DEBUG
#    		CFLAGS_OPTIMIZE
#    		CFLAGS_WARNING
#    		LDFLAGS_DEBUG
#    		LDFLAGS_OPTIMIZE
#    		PATHTYPE
#    		CC_OBJNAME
#    		CC_EXENAME
#
#	Defines the following vars for non-gcc compilers
#    		SHLIB_LD
#    		SHLIB_LD_LIBS
#    		LIBS
#    		AR
#    		MAKE_LIB
#    		MAKE_EXE
#    		MAKE_DLL
#
#    		LIBSUFFIX
#    		LIBRARIES
#    		EXESUFFIX
#    		DLLSUFFIX
#
#--------------------------------------------------------------------

AC_DEFUN(SC_CONFIG_CFLAGS, [
    AC_MSG_CHECKING([compiler flags])
    EXTRA_CFLAGS=""
    # set various compiler flags depending on whether we are using gcc or cl
    
    if test "${GCC}" = "yes" ; then
	CFLAGS_DEBUG=-g
	CFLAGS_OPTIMIZE=-O
	CFLAGS_WARNING="-Wall -Wconversion"
	LDFLAGS_DEBUG=-g
	LDFLAGS_OPTIMIZE=-O
	PATHTYPE=-u
	
	# Specify the CC output file names based on the target name
	CC_OBJNAME="-o \[$]@"
	CC_EXENAME="-o \[$]@"
    else
	SHLIB_LD="link -dll -nologo"
	SHLIB_LD_LIBS="user32.lib advapi32.lib"
	LIBS="user32.lib advapi32.lib"
	AR="lib -nologo"
	RC="rc"
	MAKE_LIB="\${AR} -out:\[$]@"
	MAKE_EXE="\${CC} -Fe\[$]@"
	
	if test "${SHARED_BUILD}" = "0" ; then
	    # static
            AC_MSG_RESULT([using static flags])
	    runtime=-MT
	    MAKE_DLL="echo "
	    LIBSUFFIX="s\${DBGX}.lib"
	    LIBRARIES="\${STATIC_LIBRARIES}"
	    EXESUFFIX="s\${DBGX}.exe"
	    DLLSUFFIX=""
	else
	    # dynamic
            AC_MSG_RESULT([using shared flags])
	    runtime=-MD
	    MAKE_DLL="\${SHLIB_LD} \${SHLIB_LD_LIBS} \$(LDFLAGS) -out:\[$]@"
	    LIBSUFFIX="\${DBGX}.lib"
	    DLLSUFFIX="\${DBGX}.dll"
	    EXESUFFIX="\${DBGX}.exe"
	    LIBRARIES="\${SHARED_LIBRARIES}"
	fi

	EXTRA_CFLAGS="-YX"
	CFLAGS_DEBUG="-nologo -Z7 -Od -WX ${runtime}d"
#	CFLAGS_OPTIMIZE="-nologo -O2 -Gs -GD ${runtime}"
	CFLAGS_OPTIMIZE="-nologo -Oti -Gs -GD ${runtime}"
	CFLAGS_WARNING="-W3"
	LDFLAGS_DEBUG="-debug:full -debugtype:cv"
	LDFLAGS_OPTIMIZE="-release"
	PATHTYPE=-w
	
	# Specify the CC output file names based on the target name
	CC_OBJNAME="-Fo\[$]@"
	CC_EXENAME="-Fe\"\$(shell cygpath \$(PATHTYPE) '\[$]@')\""

	# Specify linker flags depending on the type of app being 
	# built -- Console vs. Window.
	LDFLAGS_CONSOLE="-subsystem:console"
	LDFLAGS_WINDOW="-subsystem:windows"
    fi
])

#------------------------------------------------------------------------
# SC_WITH_TCL --
#
#	Location of the Tcl build directory.
#
# Arguments:
#	none
#
# Results:
#
#	Adds the following arguments to configure:
#		--with-tcl=...
#
#	Defines the following vars:
#		TCL_BIN_DIR	Full path to the tcl build dir.
#------------------------------------------------------------------------

AC_DEFUN(SC_WITH_TCL, [
    if test -d ../../tcl8.2$1/win;  then
	TCL_BIN_DEFAULT=../../tcl8.2$1/win
    else
	TCL_BIN_DEFAULT=../../tcl8.2/win
    fi
    
    AC_ARG_WITH(tcl, [  --with-tcl=DIR          use Tcl 8.2 binaries from DIR],
	    TCL_BIN_DIR=$withval, TCL_BIN_DIR=`cd $TCL_BIN_DEFAULT; pwd`)
    if test ! -d $TCL_BIN_DIR; then
	AC_MSG_ERROR(Tcl directory $TCL_BIN_DIR does not exist)
    fi
    if test ! -f $TCL_BIN_DIR/Makefile; then
	AC_MSG_ERROR(There is no Makefile in $TCL_BIN_DIR:  perhaps you did not specify the Tcl *build* directory (not the toplevel Tcl directory) or you forgot to configure Tcl?)
    else
	echo "building against Tcl binaries in: $TCL_BIN_DIR"
    fi
    AC_SUBST(TCL_BIN_DIR)
])

