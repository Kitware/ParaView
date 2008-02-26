cmake_minimum_required(VERSION 2.4.5 FATAL_ERROR)

project(Python)

# user configurable options

option(WITH_TSC "profile with the Pentium timestamp counter" 0)
option(ENABLE_IPV6 "Enable IP v6" 1)
option(WITH_DOC_STRINGS "Enable if you want documentation strings in extension modules" 1)
option(Py_USING_UNICODE "Enable unicode support" 1)
option(Py_DEBUG "Build debugging interpreter" 0)
option(WITH_PYMALLOC "Define if you want to compile in Python-specific mallocs" 1)
option(HAVE_LARGEFILE_SUPPORT "Large file support "1)
option(WITH_THREAD "compile in rudimentary thread support" 1)

set(PY_MAJOR 2)
set(PY_MINOR 6)
set(PY_PATCH 0)

# do the configure checks

include(ConfigureChecks.cmake)


set(extraLibs)

set(THREAD_SRC)
if(WITH_THREAD)
  if(HAVE_LIBPTHREAD)
    set(THREAD_SRC Python/thread.c)
    set(_REENTRANT 1)
    set(extraLibs ${extraLibs} ${HAVE_LIBPTHREAD})
  endif(HAVE_LIBPTHREAD)
  if(WIN32)
    set(THREAD_SRC Python/thread.c)
  endif(WIN32)
endif(WITH_THREAD)

if(HAVE_LIBM)
  set(extraLibs ${extraLibs} ${HAVE_LIBM})
endif(HAVE_LIBM)

if(HAVE_LIBUTIL)
  set(extraLibs ${extraLibs} ${HAVE_LIBUTIL})
endif(HAVE_LIBUTIL)

set(DYNLOAD_SRC)
if(HAVE_DYNAMIC_LOADING)
  if(HAVE_LIBDL)
    set(extraLibs ${extraLibs} ${HAVE_LIBDL})
    set(DYNLOAD_SRC ${DYNLOAD_SRC} Python/dynload_shlib.c)
  endif(HAVE_LIBDL)
  if(WIN32)
    set(DYNLOAD_SRC ${DYNLOAD_SRC} Python/dynload_win.c PC/import_nt.c PC/dl_nt.c)
  endif(WIN32)
  set(BUILD_LIBS_SHARED_DEFAULT TRUE)
else(HAVE_DYNAMIC_LOADING)
  set(BUILD_LIBS_SHARED_DEFAULT FALSE)
endif(HAVE_DYNAMIC_LOADING)

option(PYTHON_BUILD_LIB_SHARED "Build libpython shared" ${BUILD_LIBS_SHARED_DEFAULT})
if(PYTHON_BUILD_LIB_SHARED)
  set(LIB_BUILD_TYPE SHARED)
else(PYTHON_BUILD_LIB_SHARED)
  set(LIB_BUILD_TYPE STATIC)
endif(PYTHON_BUILD_LIB_SHARED)

set(MACHDEP_SRCS )   #TODO: configure

set(SIGNAL_SRCS )    #TODO: configure

set(MOD_SRCS )


#####################################
#
# setup modules
#
#####################################

set(STATIC_MODULES)
set(DYNAMIC_MODULES)

macro(ADD_PYTHON_MODULE _NAME _STATIC_SRCS)
  option(MODULE_${_NAME}_ENABLE "Add module ${_NAME}" TRUE)

  if(MODULE_${_NAME}_ENABLE)
    option(MODULE_${_NAME}_SHARED "Add module ${_NAME} shared" ${BUILD_LIBS_SHARED_DEFAULT})
    if(MODULE_${_NAME}_SHARED)
      message(STATUS "Adding ${_NAME} as shared module")
      set(DYNAMIC_MODULES ${DYNAMIC_MODULES} ${_NAME})
      add_library(${_NAME} MODULE ${ARGN})
      set_target_properties(${_NAME} PROPERTIES PREFIX "")
    else(MODULE_${_NAME}_SHARED)
      message(STATUS "Adding ${_NAME} as static module")
      set(STATIC_MODULES ${STATIC_MODULES} ${_NAME})
      set(${_STATIC_SRCS} ${${_STATIC_SRCS}} ${ARGN})
      set(PY_STATIC_MODULES_INIT_FUNCTIONS "${PY_STATIC_MODULES_INIT_FUNCTIONS}extern void init${_NAME}(void);\n")
      set(PY_STATIC_MODULES_INITTAB        "${PY_STATIC_MODULES_INITTAB}  {\"${_NAME}\", init${_NAME}},\n")
    endif(MODULE_${_NAME}_SHARED)
  endif(MODULE_${_NAME}_ENABLE)
endmacro(ADD_PYTHON_MODULE)

if(WITH_THREAD)
  add_python_module(thread  MOD_SRCS Modules/threadmodule.c)
endif(WITH_THREAD)

if(WIN32)
  set(PY_POSIX_NAME nt)
else(WIN32)
  set(PY_POSIX_NAME posix)
endif(WIN32)

add_python_module(${PY_POSIX_NAME}  MOD_SRCS Modules/posixmodule.c)
#add_python_module(signal    MOD_SRCS Modules/signalmodule.c)
add_python_module(_codecs   MOD_SRCS Modules/_codecsmodule.c)
add_python_module(_sre      MOD_SRCS Modules/_sre.c)
add_python_module(_weakref  MOD_SRCS Modules/_weakref.c)
add_python_module(errno     MOD_SRCS Modules/errnomodule.c)
add_python_module(pwd       MOD_SRCS Modules/pwdmodule.c)
add_python_module(_symtable MOD_SRCS Modules/symtablemodule.c)
add_python_module(xxsubtype MOD_SRCS Modules/xxsubtype.c)
add_python_module(zipimport MOD_SRCS Modules/zipimport.c)

option(BUILD_UNTESTED_MODULES "Build extra modules" FALSE)
if(BUILD_UNTESTED_MODULES)
  add_python_module(_bisect       MOD_SRCS Modules/_bisectmodule.c )
  add_python_module(_bsddb        MOD_SRCS Modules/_bsddb.c )
  add_python_module(_collections  MOD_SRCS Modules/_collectionsmodule.c )
  add_python_module(_csv          MOD_SRCS Modules/_csv.c )
  add_python_module(_elementtree  MOD_SRCS Modules/_elementtree.c )
  add_python_module(_functools    MOD_SRCS Modules/_functoolsmodule.c )
  add_python_module(_heapq        MOD_SRCS Modules/_heapqmodule.c )
  add_python_module(_hotshot      MOD_SRCS Modules/_hotshot.c )
  add_python_module(_locale       MOD_SRCS Modules/_localemodule.c )
  add_python_module(_lsprof       MOD_SRCS Modules/_lsprof.c )
  add_python_module(_random       MOD_SRCS Modules/_randommodule.c )
  add_python_module(_struct       MOD_SRCS Modules/_struct.c )
  add_python_module(array         MOD_SRCS Modules/arraymodule.c )
  add_python_module(binascii      MOD_SRCS Modules/binascii.c )
  add_python_module(cmath         MOD_SRCS Modules/cmathmodule.c )
  add_python_module(datetime      MOD_SRCS Modules/datetimemodule.c )
  add_python_module(dl            MOD_SRCS Modules/dlmodule.c )
  add_python_module(fcntl         MOD_SRCS Modules/fcntlmodule.c )
  add_python_module(grp           MOD_SRCS Modules/grpmodule.c )
  add_python_module(imageop       MOD_SRCS Modules/imageop.c )
  add_python_module(math          MOD_SRCS Modules/mathmodule.c )
  add_python_module(operator      MOD_SRCS Modules/operator.c )
  add_python_module(parser        MOD_SRCS Modules/parsermodule.c )
endif(BUILD_UNTESTED_MODULES)


file(MAKE_DIRECTORY ${Python_BINARY_DIR}/Modules)
if(WIN32)
  configure_file(PC/pyconfig.h ${Python_BINARY_DIR}/pyconfig.h COPYONLY)
else(WIN32)
  configure_file(pyconfig.h.cmake ${Python_BINARY_DIR}/pyconfig.h @ONLY)
endif(WIN32)
configure_file(Modules/config.c.cmake ${CMAKE_CURRENT_BINARY_DIR}/Modules/modules-config.c @ONLY)
file(WRITE ${Python_BINARY_DIR}/Modules/Setup "#created by cmake, do not edit\n")


#####################################
#
# build python and libpython
# 
#####################################

set(P_SRCS      Parser/acceler.c
                Parser/grammar1.c
                Parser/listnode.c
                Parser/node.c
                Parser/parser.c
                Parser/parsetok.c
                Parser/bitset.c
                Parser/metagrammar.c
                Parser/firstsets.c
                Parser/grammar.c
                Parser/pgen.c )

set(PARSER_SRCS ${P_SRCS} Parser/myreadline.c Parser/tokenizer.c )

set(OBJECT_SRCS Objects/abstract.c
                Objects/boolobject.c
                Objects/bufferobject.c
                Objects/cellobject.c
                Objects/classobject.c
                Objects/cobject.c
                Objects/codeobject.c
                Objects/complexobject.c
                Objects/descrobject.c
                Objects/enumobject.c
                Objects/exceptions.c
                Objects/genobject.c
                Objects/fileobject.c
                Objects/floatobject.c
                Objects/frameobject.c
                Objects/funcobject.c
                Objects/intobject.c
                Objects/iterobject.c
                Objects/listobject.c
                Objects/longobject.c
                Objects/dictobject.c
                Objects/methodobject.c
                Objects/moduleobject.c
                Objects/object.c
                Objects/obmalloc.c
                Objects/rangeobject.c
                Objects/setobject.c
                Objects/sliceobject.c
                Objects/stringobject.c
                Objects/structseq.c
                Objects/tupleobject.c
                Objects/typeobject.c
                Objects/weakrefobject.c
   )

if(Py_USING_UNICODE)
   set(OBJECT_SRCS ${OBJECT_SRCS} 
                   Objects/unicodeobject.c 
                   Objects/unicodectype.c)
endif(Py_USING_UNICODE)

set(PYTHON_SRCS Python/Python-ast.c 
                Python/asdl.c 
                Python/ast.c 
                Python/bltinmodule.c 
                Python/ceval.c 
                Python/compile.c 
                Python/codecs.c 
                Python/errors.c 
                Python/formatter_string.c
                Python/formatter_unicode.c
                Python/frozen.c 
                Python/frozenmain.c 
                Python/future.c 
                Python/getargs.c 
                Python/getcompiler.c 
                Python/getcopyright.c 
                Python/getmtime.c 
                Python/getplatform.c 
                Python/getversion.c 
                Python/graminit.c 
                Python/import.c 
                Python/importdl.c 
                Python/marshal.c 
                Python/modsupport.c 
                Python/mystrtoul.c 
                Python/mysnprintf.c 
                Python/peephole.c
                Python/pyarena.c 
                Python/pyfpe.c 
                Python/pystate.c 
                Python/pythonrun.c 
                Python/structmember.c 
                Python/symtable.c 
                Python/sysmodule.c 
                Python/traceback.c 
                Python/getopt.c 
                Python/pystrcmp.c
                Python/pystrtod.c 
                ${MACHDEP_SRCS}
                ${THREAD_SRC}
                ${DYNLOAD_SRC}
                )

set(MODULE_SRCS ${CMAKE_CURRENT_BINARY_DIR}/Modules/modules-config.c 
                Modules/getbuildinfo.c 
                Modules/_typesmodule.c
                Modules/getpath.c 
                Modules/main.c 
                Modules/gcmodule.c  
                Modules/signalmodule.c  
                ${MOD_SRCS} )

set_source_files_properties(Python/getplatform.c PROPERTIES COMPILE_FLAGS -DPLATFORM=\\"${PY_PLATFORM}\\")
set_source_files_properties(Modules/getpath.c PROPERTIES COMPILE_FLAGS  "-DPREFIX=\\\"${CMAKE_INSTALL_PREFIX}\\\" -DEXEC_PREFIX=\\\"${CMAKE_INSTALL_PREFIX}\\\" -DVERSION=\\\"${PY_MAJOR}.${PY_MINOR}\\\" -DVPATH=\\\"..\\\"  -DPYTHONPATH=\\\":plat-${PY_PLATFORM}\\\" " )

include_directories(Include ${Python_BINARY_DIR})
if(WIN32)
  include_directories(PC Python)
  add_definitions(-DWIN32 -D_WINDOWS)
endif(WIN32)

add_definitions(-DPy_BUILD_CORE)

if(NOT HAVE_HYPOT)
  add_definitions(-DWITHOUT_COMPLEX)
endif(NOT HAVE_HYPOT)


add_library(pythonLib  ${LIB_BUILD_TYPE}
              ${PARSER_SRCS}
              ${OBJECT_SRCS}
              ${PYTHON_SRCS}
              ${MODULE_SRCS}
              ${SIGNAL_SRCS}
              )

target_link_libraries(pythonLib ${extraLibs})
set_target_properties(pythonLib PROPERTIES OUTPUT_NAME python${PY_MAJOR}.${PY_MINOR})

add_executable(python Modules/python.c )
target_link_libraries(python pythonLib)

#if(CMAKE_SYSTEM MATCHES BlueGene)
#  get_target_property(EXE_LOCATION python LOCATION)
#  configure_file(runcone.ksh.in ${CMAKE_CURRENT_BINARY_DIR}/runpy.ksh)
#endif(CMAKE_SYSTEM MATCHES BlueGene)

add_subdirectory(Lib)


#####################################
#
# install rules
#
#####################################

install(TARGETS python pythonLib 
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# install the python headers
file(GLOB pyHeaders ${Python_SOURCE_DIR}/Include/*.h )
install(FILES ${Python_BINARY_DIR}/pyconfig.h ${pyHeaders} DESTINATION include/python${PY_MAJOR}.${PY_MINOR})

# install the dynamic python modules
if(DYNAMIC_MODULES)
  install(TARGETS ${DYNAMIC_MODULES} DESTINATION lib/python${PY_MAJOR}.${PY_MINOR}/lib-dynload )
endif(DYNAMIC_MODULES)

# install the python config tool
set(EXENAME ${CMAKE_INSTALL_PREFIX}/bin/python)
configure_file(Misc/python-config.in ${CMAKE_CURRENT_BINARY_DIR}/python${PY_MAJOR}.${PY_MINOR}-config @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/python${PY_MAJOR}.${PY_MINOR}-config DESTINATION bin)

# man pages
install(FILES Misc/python.man DESTINATION man/man1 RENAME python.1)

install(FILES LICENSE DESTINATION lib/python${PY_MAJOR}.${PY_MINOR} RENAME LICENSE.txt)


#####################################
#
# setup packaging with CPack
#
#####################################

include(PythonCPack.cmake)
