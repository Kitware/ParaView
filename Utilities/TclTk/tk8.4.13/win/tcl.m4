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

AC_DEFUN([SC_PATH_TCLCONFIG], [
    AC_MSG_CHECKING([the location of tclConfig.sh])

    if test -d ../../tcl8.4$1/win;  then
	TCL_BIN_DIR_DEFAULT=../../tcl8.4$1/win
    elif test -d ../../tcl8.4/win;  then
	TCL_BIN_DIR_DEFAULT=../../tcl8.4/win
    else
	TCL_BIN_DIR_DEFAULT=../../tcl/win
    fi
    
    AC_ARG_WITH(tcl, [  --with-tcl=DIR          use Tcl 8.4 binaries from DIR],
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

AC_DEFUN([SC_PATH_TKCONFIG], [
    AC_MSG_CHECKING([the location of tkConfig.sh])

    if test -d ../../tk8.4$1/win;  then
	TK_BIN_DIR_DEFAULT=../../tk8.4$1/win
    elif test -d ../../tk8.4/win;  then
	TK_BIN_DIR_DEFAULT=../../tk8.4/win
    else
	TK_BIN_DIR_DEFAULT=../../tk/win
    fi
    
    AC_ARG_WITH(tk, [  --with-tk=DIR          use Tk 8.4 binaries from DIR],
	    TK_BIN_DIR=$withval, TK_BIN_DIR=`cd $TK_BIN_DIR_DEFAULT; pwd`)
    if test ! -d $TK_BIN_DIR; then
	AC_MSG_ERROR(Tk directory $TK_BIN_DIR does not exist)
    fi
    if test ! -f $TK_BIN_DIR/tkConfig.sh; then
	AC_MSG_ERROR(There is no tkConfig.sh in $TK_BIN_DIR:  perhaps you did not specify the Tk *build* directory (not the toplevel Tk directory) or you forgot to configure Tk?)
    fi

    AC_MSG_RESULT([$TK_BIN_DIR/tkConfig.sh])
])

#------------------------------------------------------------------------
# SC_LOAD_TCLCONFIG --
#
#	Load the tclConfig.sh file.
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

AC_DEFUN([SC_LOAD_TCLCONFIG], [
    AC_MSG_CHECKING([for existence of $TCL_BIN_DIR/tclConfig.sh])

    if test -f "$TCL_BIN_DIR/tclConfig.sh" ; then
        AC_MSG_RESULT([loading])
	. $TCL_BIN_DIR/tclConfig.sh
    else
        AC_MSG_RESULT([file not found])
    fi

    #
    # If the TCL_BIN_DIR is the build directory (not the install directory),
    # then set the common variable name to the value of the build variables.
    # For example, the variable TCL_LIB_SPEC will be set to the value
    # of TCL_BUILD_LIB_SPEC. An extension should make use of TCL_LIB_SPEC
    # instead of TCL_BUILD_LIB_SPEC since it will work with both an
    # installed and uninstalled version of Tcl.
    #

    if test -f $TCL_BIN_DIR/Makefile ; then
        TCL_LIB_SPEC=${TCL_BUILD_LIB_SPEC}
        TCL_STUB_LIB_SPEC=${TCL_BUILD_STUB_LIB_SPEC}
        TCL_STUB_LIB_PATH=${TCL_BUILD_STUB_LIB_PATH}
    fi

    #
    # eval is required to do the TCL_DBGX substitution
    #

    eval "TCL_LIB_FILE=\"${TCL_LIB_FILE}\""
    eval "TCL_LIB_FLAG=\"${TCL_LIB_FLAG}\""
    eval "TCL_LIB_SPEC=\"${TCL_LIB_SPEC}\""

    eval "TCL_STUB_LIB_FILE=\"${TCL_STUB_LIB_FILE}\""
    eval "TCL_STUB_LIB_FLAG=\"${TCL_STUB_LIB_FLAG}\""
    eval "TCL_STUB_LIB_SPEC=\"${TCL_STUB_LIB_SPEC}\""

    AC_SUBST(TCL_VERSION)
    AC_SUBST(TCL_BIN_DIR)
    AC_SUBST(TCL_SRC_DIR)

    AC_SUBST(TCL_LIB_FILE)
    AC_SUBST(TCL_LIB_FLAG)
    AC_SUBST(TCL_LIB_SPEC)

    AC_SUBST(TCL_STUB_LIB_FILE)
    AC_SUBST(TCL_STUB_LIB_FLAG)
    AC_SUBST(TCL_STUB_LIB_SPEC)

    AC_SUBST(TCL_DEFS)
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

AC_DEFUN([SC_LOAD_TKCONFIG], [
    AC_MSG_CHECKING([for existence of $TK_BIN_DIR/tkConfig.sh])

    if test -f "$TK_BIN_DIR/tkConfig.sh" ; then
        AC_MSG_RESULT([loading])
	. $TK_BIN_DIR/tkConfig.sh
    else
        AC_MSG_RESULT([could not find $TK_BIN_DIR/tkConfig.sh])
    fi


    AC_SUBST(TK_BIN_DIR)
    AC_SUBST(TK_SRC_DIR)
    AC_SUBST(TK_LIB_FILE)
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

AC_DEFUN([SC_ENABLE_SHARED], [
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

AC_DEFUN([SC_ENABLE_THREADS], [
    AC_MSG_CHECKING(for building with threads)
    AC_ARG_ENABLE(threads, [  --enable-threads        build with threads],
	[tcl_ok=$enableval], [tcl_ok=no])

    if test "$tcl_ok" = "yes"; then
	AC_MSG_RESULT(yes)
	TCL_THREADS=1
	AC_DEFINE(TCL_THREADS)
	# USE_THREAD_ALLOC tells us to try the special thread-based
	# allocator that significantly reduces lock contention
	AC_DEFINE(USE_THREAD_ALLOC)
    else
	TCL_THREADS=0
	AC_MSG_RESULT([no (default)])
    fi
    AC_SUBST(TCL_THREADS)
])

#------------------------------------------------------------------------
# SC_ENABLE_SYMBOLS --
#
#	Specify if debugging symbols should be used
#	Memory (TCL_MEM_DEBUG) and compile (TCL_COMPILE_DEBUG) debugging
#	can also be enabled.
#
# Arguments:
#	none
#	
#	Requires the following vars to be set in the Makefile:
#		CFLAGS_DEBUG
#		CFLAGS_OPTIMIZE
#	
# Results:
#
#	Adds the following arguments to configure:
#		--enable-symbols
#
#	Defines the following vars:
#		CFLAGS_DEFAULT	Sets to $(CFLAGS_DEBUG) if true
#				Sets to $(CFLAGS_OPTIMIZE) if false
#		LDFLAGS_DEFAULT	Sets to $(LDFLAGS_DEBUG) if true
#				Sets to $(LDFLAGS_OPTIMIZE) if false
#		DBGX		Debug library extension
#
#------------------------------------------------------------------------

AC_DEFUN([SC_ENABLE_SYMBOLS], [
    AC_MSG_CHECKING([for build with symbols])
    AC_ARG_ENABLE(symbols, [  --enable-symbols        build with debugging symbols [--disable-symbols]],    [tcl_ok=$enableval], [tcl_ok=no])
# FIXME: Currently, LDFLAGS_DEFAULT is not used, it should work like CFLAGS_DEFAULT.
    if test "$tcl_ok" = "no"; then
	CFLAGS_DEFAULT='$(CFLAGS_OPTIMIZE)'
	LDFLAGS_DEFAULT='$(LDFLAGS_OPTIMIZE)'
	DBGX=""
	AC_MSG_RESULT([no])
    else
	CFLAGS_DEFAULT='$(CFLAGS_DEBUG)'
	LDFLAGS_DEFAULT='$(LDFLAGS_DEBUG)'
	DBGX=g
	if test "$tcl_ok" = "yes"; then
	    AC_MSG_RESULT([yes (standard debugging)])
	fi
    fi
    AC_SUBST(CFLAGS_DEFAULT)
    AC_SUBST(LDFLAGS_DEFAULT)

    if test "$tcl_ok" = "mem" -o "$tcl_ok" = "all"; then
	AC_DEFINE(TCL_MEM_DEBUG)
    fi

    if test "$tcl_ok" = "compile" -o "$tcl_ok" = "all"; then
	AC_DEFINE(TCL_COMPILE_DEBUG)
	AC_DEFINE(TCL_COMPILE_STATS)
    fi

    if test "$tcl_ok" != "yes" -a "$tcl_ok" != "no"; then
	if test "$tcl_ok" = "all"; then
	    AC_MSG_RESULT([enabled symbols mem compile debugging])
	else
	    AC_MSG_RESULT([enabled $tcl_ok debugging])
	fi
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
#	Can the following vars:
#		EXTRA_CFLAGS
#		CFLAGS_DEBUG
#		CFLAGS_OPTIMIZE
#		CFLAGS_WARNING
#		LDFLAGS_DEBUG
#		LDFLAGS_OPTIMIZE
#		LDFLAGS_CONSOLE
#		LDFLAGS_WINDOW
#		CC_OBJNAME
#		CC_EXENAME
#		CYGPATH
#		STLIB_LD
#		SHLIB_LD
#		SHLIB_LD_LIBS
#		LIBS
#		AR
#		RC
#		RES
#
#		MAKE_LIB
#		MAKE_EXE
#		MAKE_DLL
#
#		LIBSUFFIX
#		LIBPREFIX
#		LIBRARIES
#		EXESUFFIX
#		DLLSUFFIX
#
#--------------------------------------------------------------------

AC_DEFUN([SC_CONFIG_CFLAGS], [

    # Step 0: Enable 64 bit support?

    AC_MSG_CHECKING([if 64bit support is requested])
    AC_ARG_ENABLE(64bit,[  --enable-64bit          enable 64bit support (where applicable = amd64|ia64)], [do64bit=$enableval], [do64bit=no])
    AC_MSG_RESULT($do64bit)

    # Set some defaults (may get changed below)
    EXTRA_CFLAGS=""

    AC_CHECK_PROG(CYGPATH, cygpath, cygpath -w, echo)

    SHLIB_SUFFIX=".dll"

    # MACHINE is IX86 for LINK, but this is used by the manifest,
    # which requires x86|amd64|ia64.
    MACHINE="X86"

    # Check for a bug in gcc's windres that causes the
    # compile to fail when a Windows native path is
    # passed into windres. The mingw toolchain requires
    # Windows native paths while Cygwin should work
    # with both. Avoid the bug by passing a POSIX
    # path when using the Cygwin toolchain.

    if test "$GCC" = "yes" && test "$CYGPATH" != "echo" ; then
	conftest=/tmp/conftest.rc
	echo "STRINGTABLE BEGIN" > $conftest
	echo "101 \"name\"" >> $conftest
	echo "END" >> $conftest

	AC_MSG_CHECKING([for Windows native path bug in windres])
	cyg_conftest=`$CYGPATH $conftest`
	if AC_TRY_COMMAND($RC -o conftest.res.o $cyg_conftest) ; then
	    AC_MSG_RESULT([no])
	else
	    AC_MSG_RESULT([yes])
	    CYGPATH=echo
	fi
	conftest=
	cyg_conftest=
    fi

    if test "$CYGPATH" = "echo" || test "$ac_cv_cygwin" = "yes"; then
        DEPARG='"$<"'
    else
        DEPARG='"$(shell $(CYGPATH) $<)"'
    fi

    # set various compiler flags depending on whether we are using gcc or cl

    AC_MSG_CHECKING([compiler flags])
    if test "${GCC}" = "yes" ; then
	if test "$do64bit" != "no" ; then
	    AC_MSG_WARN("64bit mode not supported with GCC on Windows")
	fi
	SHLIB_LD=""
	SHLIB_LD_LIBS=""
	LIBS=""
	LIBS_GUI="-lgdi32 -lcomdlg32 -limm32 -lcomctl32 -lshell32"
	STLIB_LD='${AR} cr'
	RC_OUT=-o
	RC_TYPE=
	RC_INCLUDE=--include
	RC_DEFINE=--define
	RES=res.o
	MAKE_LIB="\${STLIB_LD} \[$]@"
	POST_MAKE_LIB="\${RANLIB} \[$]@"
	MAKE_EXE="\${CC} -o \[$]@"
	LIBPREFIX="lib"

	#if test "$ac_cv_cygwin" = "yes"; then
	#    extra_cflags="-mno-cygwin"
	#    extra_ldflags="-mno-cygwin"
	#else
	#    extra_cflags=""
	#    extra_ldflags=""
	#fi

	if test "$ac_cv_cygwin" = "yes"; then
	  touch ac$$.c
	  if ${CC} -c -mwin32 ac$$.c >/dev/null 2>&1; then
	    case "$extra_cflags" in
	      *-mwin32*) ;;
	      *) extra_cflags="-mwin32 $extra_cflags" ;;
	    esac
	    case "$extra_ldflags" in
	      *-mwin32*) ;;
	      *) extra_ldflags="-mwin32 $extra_ldflags" ;;
	    esac
	  fi
	  rm -f ac$$.o ac$$.c
	else
	  extra_cflags=''
	  extra_ldflags=''
	fi

	if test "${SHARED_BUILD}" = "0" ; then
	    # static
            AC_MSG_RESULT([using static flags])
	    runtime=
	    MAKE_DLL="echo "
	    LIBSUFFIX="s\${DBGX}.a"
	    LIBFLAGSUFFIX="s\${DBGX}"
	    LIBRARIES="\${STATIC_LIBRARIES}"
	    EXESUFFIX="s\${DBGX}.exe"
	else
	    # dynamic
            AC_MSG_RESULT([using shared flags])

	    # ad-hoc check to see if CC supports -shared.
	    if "${CC}" -shared 2>&1 | egrep ': -shared not supported' >/dev/null; then
		AC_MSG_ERROR([${CC} does not support the -shared option.
                You will need to upgrade to a newer version of the toolchain.])
	    fi

	    runtime=
	    # Link with gcc since ld does not link to default libs like
	    # -luser32 and -lmsvcrt by default. Make sure CFLAGS is
	    # included so -mno-cygwin passed the correct libs to the linker.
	    SHLIB_LD='${CC} -shared ${CFLAGS}'
	    SHLIB_LD_LIBS='${LIBS}'
	    # Add SHLIB_LD_LIBS to the Make rule, not here.
	    MAKE_DLL="\${SHLIB_LD} \$(LDFLAGS) -o \[$]@ ${extra_ldflags} \
	        -Wl,--out-implib,\$(patsubst %.dll,lib%.a,\[$]@)"

	    LIBSUFFIX="\${DBGX}.a"
	    LIBFLAGSUFFIX="\${DBGX}"
	    EXESUFFIX="\${DBGX}.exe"
	    LIBRARIES="\${SHARED_LIBRARIES}"
	fi
	# DLLSUFFIX is separate because it is the building block for
	# users of tclConfig.sh that may build shared or static.
	DLLSUFFIX="\${DBGX}.dll"
	SHLIB_SUFFIX=.dll

	EXTRA_CFLAGS="${extra_cflags}"

	CFLAGS_DEBUG=-g
	CFLAGS_OPTIMIZE="-O2 -fomit-frame-pointer"
	CFLAGS_WARNING="-Wall -Wconversion"
	LDFLAGS_DEBUG=
	LDFLAGS_OPTIMIZE=

	# Specify the CC output file names based on the target name
	CC_OBJNAME="-o \[$]@"
	CC_EXENAME="-o \[$]@"

	# Specify linker flags depending on the type of app being 
	# built -- Console vs. Window.
	#
	# ORIGINAL COMMENT:
	# We need to pass -e _WinMain@16 so that ld will use
	# WinMain() instead of main() as the entry point. We can't
	# use autoconf to check for this case since it would need
	# to run an executable and that does not work when
	# cross compiling. Remove this -e workaround once we
	# require a gcc that does not have this bug.
	#
	# MK NOTE: Tk should use a different mechanism. This causes 
	# interesting problems, such as wish dying at startup.
	#LDFLAGS_WINDOW="-mwindows -e _WinMain@16 ${extra_ldflags}"
	LDFLAGS_CONSOLE="-mconsole ${extra_ldflags}"
	LDFLAGS_WINDOW="-mwindows ${extra_ldflags}"
    else
	if test "${SHARED_BUILD}" = "0" ; then
	    # static
            AC_MSG_RESULT([using static flags])
	    runtime=-MT
	    MAKE_DLL="echo "
	    LIBSUFFIX="s\${DBGX}.lib"
	    LIBFLAGSUFFIX="s\${DBGX}"
	    LIBRARIES="\${STATIC_LIBRARIES}"
	    EXESUFFIX="s\${DBGX}.exe"
	    SHLIB_LD_LIBS=""
	else
	    # dynamic
            AC_MSG_RESULT([using shared flags])
	    runtime=-MD
	    # Add SHLIB_LD_LIBS to the Make rule, not here.
	    MAKE_DLL="\${SHLIB_LD} \$(LDFLAGS) -out:\[$]@"
	    LIBSUFFIX="\${DBGX}.lib"
	    LIBFLAGSUFFIX="\${DBGX}"
	    EXESUFFIX="\${DBGX}.exe"
	    LIBRARIES="\${SHARED_LIBRARIES}"
	    SHLIB_LD_LIBS='${LIBS}'
	fi
	# DLLSUFFIX is separate because it is the building block for
	# users of tclConfig.sh that may build shared or static.
	DLLSUFFIX="\${DBGX}.dll"

	# This is a 2-stage check to make sure we have the 64-bit SDK
	# We have to know where the SDK is installed.
	# This magic is based on MS Platform SDK for Win2003 SP1 - hobbs
	if test "$do64bit" != "no" ; then
	    if test "x${MSSDK}x" = "xx" ; then
		MSSDK="C:/Progra~1/Microsoft Platform SDK"
	    fi
	    MSSDK=`echo "$MSSDK" | sed -e 's!\\\!/!g'`
	    PATH64=""
	    case "$do64bit" in
		amd64|x64|yes)
		    MACHINE="AMD64" ; # default to AMD64 64-bit build
		    PATH64="${MSSDK}/Bin/Win64/x86/AMD64"
		    ;;
		ia64)
		    MACHINE="IA64"
		    PATH64="${MSSDK}/Bin/Win64"
		    ;;
	    esac
	    if test ! -d "${PATH64}" ; then
		AC_MSG_WARN([Could not find 64-bit $MACHINE SDK to enable 64bit mode])
		AC_MSG_WARN([Ensure latest Platform SDK is installed])
		do64bit="no"
	    else
		AC_MSG_RESULT([   Using 64-bit $MACHINE mode])
	    fi
	fi

	if test "$do64bit" != "no" ; then
	    # The space-based-path will work for the Makefile, but will
	    # not work if AC_TRY_COMPILE is called.
	    CC="\"${PATH64}/cl.exe\" -I\"${MSSDK}/Include\" \
		-I\"${MSSDK}/Include/crt\" -I\"${MSSDK}/Include/crt/sys\""
	    RC="\"${MSSDK}/bin/rc.exe\""
	    CFLAGS_DEBUG="-nologo -Zi -Od ${runtime}d"
	    # Do not use -O2 for Win64 - this has proved buggy in code gen.
	    CFLAGS_OPTIMIZE="-nologo -O1 ${runtime}"
	    lflags="-nologo -MACHINE:${MACHINE} -LIBPATH:\"${MSSDK}/Lib/${MACHINE}\""
	    LINKBIN="\"${PATH64}/link.exe\""
	    # Avoid 'unresolved external symbol __security_cookie' errors.
	    # c.f. http://support.microsoft.com/?id=894573
	    LIBS="user32.lib advapi32.lib bufferoverflowU.lib"
	else
	    RC="rc"
	    # -Od - no optimization
	    # -WX - warnings as errors
	    CFLAGS_DEBUG="-nologo -Z7 -Od -WX ${runtime}d"
	    # -O2 - create fast code (/Og /Oi /Ot /Oy /Ob2 /Gs /GF /Gy)
	    CFLAGS_OPTIMIZE="-nologo -O2 ${runtime}"
	    lflags="-nologo"
	    LINKBIN="link"
	    LIBS="user32.lib advapi32.lib"
	fi

	LIBS_GUI="gdi32.lib comdlg32.lib imm32.lib comctl32.lib shell32.lib"
	SHLIB_LD="${LINKBIN} -dll -incremental:no ${lflags}"
	# link -lib only works when -lib is the first arg
	STLIB_LD="${LINKBIN} -lib ${lflags}"
	RC_OUT=-fo
	RC_TYPE=-r
	RC_INCLUDE=-i
	RC_DEFINE=-d
	RES=res
	MAKE_LIB="\${STLIB_LD} -out:\[$]@"
	POST_MAKE_LIB=
	MAKE_EXE="\${CC} -Fe\[$]@"
	LIBPREFIX=""

	EXTRA_CFLAGS=""
	CFLAGS_WARNING="-W3"
	LDFLAGS_DEBUG="-debug:full"
	LDFLAGS_OPTIMIZE="-release"
	
	# Specify the CC output file names based on the target name
	CC_OBJNAME="-Fo\[$]@"
	CC_EXENAME="-Fe\"\$(shell \$(CYGPATH) '\[$]@')\""

	# Specify linker flags depending on the type of app being 
	# built -- Console vs. Window.
	LDFLAGS_CONSOLE="-link -subsystem:console ${lflags}"
	LDFLAGS_WINDOW="-link -subsystem:windows ${lflags}"
    fi

    # DL_LIBS is empty, but then we match the Unix version
    AC_SUBST(DL_LIBS)
    AC_SUBST(CFLAGS_DEBUG)
    AC_SUBST(CFLAGS_OPTIMIZE)
    AC_SUBST(CFLAGS_WARNING)
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

AC_DEFUN([SC_WITH_TCL], [
    if test -d ../../tcl8.4$1/win;  then
	TCL_BIN_DEFAULT=../../tcl8.4$1/win
    else
	TCL_BIN_DEFAULT=../../tcl8.4/win
    fi
    
    AC_ARG_WITH(tcl, [  --with-tcl=DIR          use Tcl 8.4 binaries from DIR],
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

#------------------------------------------------------------------------
# SC_PROG_TCLSH
#	Locate a tclsh shell installed on the system path. This macro
#	will only find a Tcl shell that already exists on the system.
#	It will not find a Tcl shell in the Tcl build directory or
#	a Tcl shell that has been installed from the Tcl build directory.
#	If a Tcl shell can't be located on the PATH, then TCLSH_PROG will
#	be set to "". Extensions should take care not to create Makefile
#	rules that are run by default and depend on TCLSH_PROG. An
#	extension can't assume that an executable Tcl shell exists at
#	build time.
#
# Arguments
#	none
#
# Results
#	Subst's the following values:
#		TCLSH_PROG
#------------------------------------------------------------------------

AC_DEFUN([SC_PROG_TCLSH], [
    AC_MSG_CHECKING([for tclsh])

    AC_CACHE_VAL(ac_cv_path_tclsh, [
	search_path=`echo ${PATH} | sed -e 's/:/ /g'`
	for dir in $search_path ; do
	    for j in `ls -r $dir/tclsh[[8-9]]*.exe 2> /dev/null` \
		    `ls -r $dir/tclsh* 2> /dev/null` ; do
		if test x"$ac_cv_path_tclsh" = x ; then
		    if test -f "$j" ; then
			ac_cv_path_tclsh=$j
			break
		    fi
		fi
	    done
	done
    ])

    if test -f "$ac_cv_path_tclsh" ; then
	TCLSH_PROG="$ac_cv_path_tclsh"
	AC_MSG_RESULT($TCLSH_PROG)
    else
	# It is not an error if an installed version of Tcl can't be located.
	TCLSH_PROG=""
	AC_MSG_RESULT([No tclsh found on PATH])
    fi
    AC_SUBST(TCLSH_PROG)
])

#------------------------------------------------------------------------
# SC_BUILD_TCLSH
#	Determine the fully qualified path name of the tclsh executable
#	in the Tcl build directory. This macro will correctly determine
#	the name of the tclsh executable even if tclsh has not yet
#	been built in the build directory. The build tclsh must be used
#	when running tests from an extension build directory. It is not
#	correct to use the TCLSH_PROG in cases like this.
#
# Arguments
#	none
#
# Results
#	Subst's the following values:
#		BUILD_TCLSH
#------------------------------------------------------------------------

AC_DEFUN([SC_BUILD_TCLSH], [
    AC_MSG_CHECKING([for tclsh in Tcl build directory])
    BUILD_TCLSH=${TCL_BIN_DIR}/tclsh${TCL_MAJOR_VERSION}${TCL_MINOR_VERSION}${TCL_DBGX}${EXEEXT}
    AC_MSG_RESULT($BUILD_TCLSH)
    AC_SUBST(BUILD_TCLSH)
])

