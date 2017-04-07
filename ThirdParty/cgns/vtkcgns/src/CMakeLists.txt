
##########
# Global #
##########

if (WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif (WIN32)

# Include the local directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
if (CGNS_ENABLE_HDF5)
  add_definitions(-DBUILD_HDF5)
  # XXX(kitware): Handled by ParaView.
  if (FALSE)
  if (WIN32)
    if (HDF5_LIBRARY)
      file(STRINGS ${HDF5_LIBRARY} HDF5isDLL REGEX "__imp__H5close")
      if (HDF5isDLL)
        # this is only needed when using hdf5 dll libs
        add_definitions(-DWIN32 -D_HDF5USEDLL_)
      endif (HDF5isDLL)
    endif (HDF5_LIBRARY)
  endif (WIN32)
  if(HDF5_NEED_MPI AND MPI_INC)
    include_directories(${MPI_INC})
  endif(HDF5_NEED_MPI AND MPI_INC)
  endif ()
  if (CGNS_ENABLE_PARALLEL)
    add_definitions(-DBUILD_PARALLEL)
  endif (CGNS_ENABLE_PARALLEL)
endif (CGNS_ENABLE_HDF5)

################################
# create configed header files #
################################

include(CheckTypeSize)
CHECK_TYPE_SIZE(long SIZE_OF_LONG)
if ("${SIZE_OF_LONG}" GREATER 4)
  set(CGLONGT long)
else ("${SIZE_OF_LONG}" GREATER 4)
  set(CGLONGT "long long")
endif ("${SIZE_OF_LONG}" GREATER 4)

set(BUILDLEGACY 0)
set(BUILD64BIT 0)
set(BUILDSCOPE 0)
set(BUILDFORTRAN 0)
set(BUILDBASESCOPE 0)

if (CGNS_ENABLE_LEGACY)
  set(BUILDLEGACY 1)
  set(CGFSIZET "")
else (CGNS_ENABLE_LEGACY)
  if (CGNS_ENABLE_64BIT)
    set(BUILD64BIT 1)
    set(CGFSIZET *8)
  else (CGNS_ENABLE_64BIT)
    set(CGFSIZET *4)
  endif (CGNS_ENABLE_64BIT)
endif (CGNS_ENABLE_LEGACY)
if (CGNS_ENABLE_SCOPING)
  set(BUILDSCOPE 1)
endif (CGNS_ENABLE_SCOPING)
if (CGNS_ENABLE_FORTRAN)
  set(BUILDFORTRAN 1)
endif (CGNS_ENABLE_FORTRAN)
if (CGNS_ENABLE_BASE_SCOPE)
  set(BUILDBASESCOPE 1)
endif (CGNS_ENABLE_BASE_SCOPE)

# XXX(kitware): ParaView handles flags better.
if (FALSE)
set(SYSCFLAGS "")
set(CFGFLAGS "")
set(LIBS "")

file(TO_NATIVE_PATH ${CMAKE_INSTALL_PREFIX}/lib LIBDIR)
file(TO_NATIVE_PATH ${CMAKE_INSTALL_PREFIX}/include INCLUDEDIR)
file(TO_NATIVE_PATH ${CMAKE_C_COMPILER} CC)
if (WIN32)
  set(CGNSLIB cgns.lib)
else (WIN32)
  set(CGNSLIB libcgns.a)
  set(LIBS -lm)
endif (WIN32)

string(COMPARE EQUAL ${CMAKE_BUILD_TYPE} Debug IS_DEBUG)
if (IS_DEBUG)
  set(BUILDDEBUG 1)
  set(CFLAGS ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG})
  set(LDFLAGS ${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_DEBUG})
else (IS_DEBUG)
  set(BUILDDEBUG 0)
  set(CFLAGS ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE})
  set(LDFLAGS ${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_RELEASE})
endif (IS_DEBUG)
endif ()

set(BUILDHDF5 0)
set(H5NEEDMPI 0)
set(H5NEEDSZIP 0)
set(H5NEEDZLIB 0)
set(BUILDPARALLEL 0)
set(HDF5INC "")
set(HDF5LIB "")
set(SZIPLIB "")
set(ZLIBLIB "")
set(MPIINC "")
set(MPILIBS "")
set(HAVE_FORTRAN_95 0)
set(HAVE_FORTRAN_2003 0)
set(HAVE_FORTRAN_2008 0)
set(HAVE_FORTRAN_2008TS 0)
set(HDF5_HAVE_MULTI_DATASETS 0)
set(FORTRAN_DEFAULT_INTEGER_C_INT64_T 0)

if (BUILDFORTRAN)

#----------- Determine if the Fortran compiler pgi ------------------------
# The PGI compiler segfaults wehn va_start is called, so we need to add
# a special compiler flag, see CGNS-40.
#--------------------------------------------------------------------------
IF (CMAKE_C_COMPILER_ID MATCHES "PGI")
 SET(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -Mx,125,0x200")
ENDIF()

ENABLE_LANGUAGE(Fortran)

#-----------------------------------------------------------------------------
# The provided CMake Fortran macros don't provide a general check function
# so use this one.
#-----------------------------------------------------------------------------
MACRO (CHECK_FORTRAN_FEATURE FUNCTION CODE VARIABLE)
  IF (NOT DEFINED ${VARIABLE})
    IF (CMAKE_REQUIRED_LIBRARIES)
      SET (CHECK_FUNCTION_EXISTS_ADD_LIBRARIES
          "-DLINK_LIBRARIES:STRING=${CMAKE_REQUIRED_LIBRARIES}")
    ELSE (CMAKE_REQUIRED_LIBRARIES)
      SET (CHECK_FUNCTION_EXISTS_ADD_LIBRARIES)
    ENDIF (CMAKE_REQUIRED_LIBRARIES)
    FILE (WRITE
        ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/testFortranCompiler.f90
        "${CODE}"
    )
    TRY_COMPILE (${VARIABLE}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/testFortranCompiler.f90
        CMAKE_FLAGS "${CHECK_FUNCTION_EXISTS_ADD_LIBRARIES}"
        OUTPUT_VARIABLE OUTPUT
    )

#    MESSAGE ( "* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * ")
#    MESSAGE ( "Test result ${OUTPUT}")
#    MESSAGE ( "* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * ")

    IF (${VARIABLE})
      SET (${VARIABLE} 1 CACHE INTERNAL "Have Fortran function ${FUNCTION}")
      MESSAGE (STATUS "Testing Fortran ${FUNCTION} - OK")
      FILE (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
          "Determining if the Fortran ${FUNCTION} exists passed with the following output:\n"
          "${OUTPUT}\n\n"
      )
    ELSE (${VARIABLE})
      MESSAGE (STATUS "Testing Fortran ${FUNCTION} - Fail")
      SET (${VARIABLE} "" CACHE INTERNAL "Have Fortran function ${FUNCTION}")
      FILE (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
          "Determining if the Fortran ${FUNCTION} exists failed with the following output:\n"
          "${OUTPUT}\n\n")
    ENDIF (${VARIABLE})
  ENDIF (NOT DEFINED ${VARIABLE})
ENDMACRO (CHECK_FORTRAN_FEATURE)

#-----------------------------------------------------------------------------
# Configure Checks which require Fortran compilation must go in here
# not in the main ConfigureChecks.cmake files, because if the user has
# no Fortran compiler, problems arise.
#
#-----------------------------------------------------------------------------

CHECK_FORTRAN_FEATURE(Fortran2003
  "
       PROGRAM testf03
  	 USE iso_c_binding
  	 IMPLICIT NONE
  	 INTEGER(C_INT) :: a
  	 INTEGER(C_INT32_T) :: b
  	 INTEGER(C_INT64_T) :: c
         REAL(C_FLOAT) :: d
  	 REAL(C_DOUBLE) :: e
  	 CHARACTER(LEN=1,KIND=C_CHAR), TARGET :: f
  	 TYPE(C_PTR) :: g
         INTERFACE 
           INTEGER(C_INT) FUNCTION test(A, UserDataName1) BIND(C, name='cg_goto')
             USE ISO_C_BINDING
             INTEGER(C_INT)   , INTENT(IN), VALUE :: A
             CHARACTER(C_CHAR), DIMENSION(*), INTENT(IN) :: UserDataName1
           END FUNCTION test
         END INTERFACE
  	 g = C_LOC(f(1:1))
      END PROGRAM testf03 
  "
  CHECK_FORTRAN_2003
)
if (CHECK_FORTRAN_2003)
  set(HAVE_FORTRAN_2003 1)
endif (CHECK_FORTRAN_2003)
   

if (HAVE_FORTRAN_2003)
CHECK_FORTRAN_FEATURE(Fortran2008TS
  " 
       PROGRAM testf03
         USE ISO_C_BINDING
         INTERFACE 
            INTEGER(C_INT) FUNCTION test(A) BIND(C, name='test')
              USE ISO_C_BINDING
              INTEGER(C_INT), OPTIONAL :: A
            END FUNCTION test
         END INTERFACE
       END PROGRAM testf03 
  "
  CHECK_FORTRAN_2008TS
)
if (CHECK_FORTRAN_2008TS)
  set(HAVE_FORTRAN_2008TS 1)
endif (CHECK_FORTRAN_2008TS)

CHECK_FORTRAN_FEATURE(Fortran2008
  " 
       MODULE test
         USE ISO_C_BINDING
         INTERFACE binding
           FUNCTION bind1(x) BIND(C, name='Cfunc')
             IMPORT :: C_FLOAT
             REAL(C_FLOAT) :: x
           END FUNCTION bind1
           FUNCTION bind2(x) BIND(C, name='Cfunc')
             IMPORT :: C_DOUBLE
             REAL(C_DOUBLE) :: x
           END FUNCTION bind2
         END INTERFACE
       END MODULE test
  "
  CHECK_FORTRAN_2008
)
if (CHECK_FORTRAN_2008)
  set(HAVE_FORTRAN_2008 1)
endif (CHECK_FORTRAN_2008)

CHECK_FORTRAN_FEATURE(Sizeof_int
  "
      MODULE mod

        INTERFACE test_integer
           MODULE PROCEDURE test_int
           MODULE PROCEDURE test_c_int64
        END INTERFACE
        
      CONTAINS
        
        SUBROUTINE test_c_int64(a)
          USE ISO_C_BINDING
          INTEGER(C_INT64_T) :: a
        END SUBROUTINE test_c_int64
        
        SUBROUTINE test_int(a)
          USE ISO_C_BINDING
          INTEGER :: a
        END SUBROUTINE test_int
        
      END MODULE mod

      PROGRAM main
        
        USE mod
        INTEGER :: a
        
        CALL test_integer(a)
        
      END PROGRAM main
  "
  CHECK_FORTRAN_DEFAULT_INTEGER_C_INT32_T
)

if (NOT CHECK_FORTRAN_DEFAULT_INTEGER_C_INT32_T)
  set(FORTRAN_DEFAULT_INTEGER_C_INT64_T 1)
endif (NOT CHECK_FORTRAN_DEFAULT_INTEGER_C_INT32_T)

else (HAVE_FORTRAN_2003)

CHECK_FORTRAN_FEATURE(Fortran95
  " 
      PROGRAM testf95
  	 IMPLICIT NONE
         INTEGER, PARAMETER :: cgint_kind = SELECTED_INT_KIND(5)
      END PROGRAM testf95
  "
  CHECK_FORTRAN_95
)

if (CHECK_FORTRAN_95)
  set(HAVE_FORTRAN_95 1)
endif (CHECK_FORTRAN_95)
endif (HAVE_FORTRAN_2003)

endif (BUILDFORTRAN)

macro(native_paths INPUT_PATH_VARIABLE RESULT_VARIABLE)
  set(${RESULT_VARIABLE} "")
  # XXX(kitware): This hard-codes full paths into the build; removing.
  if (FALSE)
  foreach(PATH ${${INPUT_PATH_VARIABLE}})
    file(TO_NATIVE_PATH ${PATH} NATIVE_PATH)
    list(APPEND ${RESULT_VARIABLE} ${NATIVE_PATH})
  endforeach(PATH ${${INPUT_PATH_VARIABLE}})
  endif ()
endmacro(native_paths)

#-----------------------------------------------------------------------------
# The provided CMake macros don't provide a general HDF5 check function
# so use this one.
#-----------------------------------------------------------------------------

MACRO (CHECK_HDF5_FEATURE FUNCTION CODE VARIABLE)
  # XXX(kitware): I don't see any HDF5 source which provide the tested features.
  IF (NOT DEFINED ${VARIABLE} AND FALSE)
    FILE (WRITE
        ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/test_HDF.c
        "${CODE}"
    )
    SET (CHECK_FUNCTION_EXISTS_ADD_INCLUDE "-DINCLUDE_DIRECTORIES:STRING=${HDF5_INCLUDE_DIR}")
    TRY_COMPILE (${VARIABLE}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/test_HDF.c
        CMAKE_FLAGS "${CHECK_FUNCTION_EXISTS_ADD_INCLUDE}"
        OUTPUT_VARIABLE OUTPUT
    )

#    MESSAGE ( "* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * ")
#    MESSAGE ( "Test result ${OUTPUT}")
#    MESSAGE ( "* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * ")

    IF (${VARIABLE})
      SET (${VARIABLE} 1 CACHE INTERNAL "Have HDF5 function ${FUNCTION}")
      MESSAGE (STATUS "Testing HDF5 ${FUNCTION} - OK")
      FILE (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
          "Determining if the HDF5 ${FUNCTION} exists passed with the following output:\n"
          "${OUTPUT}\n\n"
      )
    ELSE (${VARIABLE})
      MESSAGE (STATUS "Testing HDF5 ${FUNCTION} - NOT FOUND")
      SET (${VARIABLE} "" CACHE INTERNAL "Have HDF5 function ${FUNCTION}")
      FILE (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
          "Determining if the HDF5 ${FUNCTION} exists failed with the following output:\n"
          "${OUTPUT}\n\n")
    ENDIF (${VARIABLE})
  ENDIF ()
ENDMACRO (CHECK_HDF5_FEATURE)

if (CGNS_ENABLE_HDF5)
  set(BUILDHDF5 1)
#  if (HDF5_INCLUDE_PATH)
#    native_paths(HDF5_INCLUDE_PATH HDF5INC)
#  endif (HDF5_INCLUDE_PATH)
#  if (HDF5_LIBRARY)
#    native_paths(HDF5_LIBRARY HDF5LIB)
#  endif (HDF5_LIBRARY)
  if (HDF5_NEED_SZIP)
    set(H5NEEDSZIP 1)
    if (SZIP_LIBRARY)
      native_paths(SZIP_LIBRARY SZIPLIB)
    endif (SZIP_LIBRARY)
  endif (HDF5_NEED_SZIP)
  if (HDF5_NEED_ZLIB)
    set(H5NEEDZLIB 1)
    if (ZLIB_LIBRARY)
      native_paths(${ZLIB_LIBRARY} ZLIBLIB)
    endif (ZLIB_LIBRARY)
  endif (HDF5_NEED_ZLIB)
  if (HDF5_NEED_MPI)
    set(H5NEEDMPI 1)
    if (MPI_INC)
      native_paths(MPI_INC MPIINC)
    endif (MPI_INC)
    if (MPI_LIBS)
      native_paths(MPI_LIBS MPILIBS)
    endif (MPI_LIBS)
    if (CGNS_ENABLE_PARALLEL)
      set(BUILDPARALLEL 1)
    endif (CGNS_ENABLE_PARALLEL)
  endif (HDF5_NEED_MPI)

  set(cgns_C_FILES ${cgns_C_FILES} adfh/ADFH.c)
  if (CGNS_ENABLE_PARALLEL)
    set(cgns_C_FILES ${cgns_C_FILES} pcgnslib.c)
  endif (CGNS_ENABLE_PARALLEL)

  # Check if HDF5 library has multi-dataset read/write APIs
  # --------------------------------------------------------------------

CHECK_HDF5_FEATURE(Multi_Dataset
  " 
 #include \"hdf5.h\"
 void main(void) 
{
  H5D_rw_multi_t *multi_info;
  return;
}
  "
   CHECK_HDF5_HAVE_MULTI_DATASETS
 )
 if (CHECK_HDF5_HAVE_MULTI_DATASETS)
   set(HDF5_HAVE_MULTI_DATASETS 1)
 endif (CHECK_HDF5_HAVE_MULTI_DATASETS)

endif (CGNS_ENABLE_HDF5)

#create these in build directory so it doesn't mess up the
#source directory, then add the path to them
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/cgnstypes.h.in
		${CMAKE_CURRENT_BINARY_DIR}/cgnstypes.h )
if (FALSE) # XXX(kitware): No fortran support.
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/cgnstypes_f.h.in
		${CMAKE_CURRENT_BINARY_DIR}/cgnstypes_f.h )
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/cgnstypes_f03.h.in
		${CMAKE_CURRENT_BINARY_DIR}/cgnstypes_f03.h )
endif ()
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/cgnsconfig.h.in
		${CMAKE_CURRENT_BINARY_DIR}/cgnsconfig.h )
if (FALSE) # XXX(kitware): No need for this.
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/cgnsBuild.defs.in
		${CMAKE_CURRENT_BINARY_DIR}/cgnsBuild.defs )
endif ()

include_directories(${CMAKE_CURRENT_BINARY_DIR})

###########
# Library #
###########

# All the C files needed by the cgns library
set(cgns_C_FILES
	cgns_error.c
	cgns_internals.c
	cgns_io.c
	cgnslib.c
	adf/ADF_interface.c
	adf/ADF_internals.c)

if (CGNS_ENABLE_HDF5)
  set(cgns_C_FILES ${cgns_C_FILES} adfh/ADFH.c)
  if (CGNS_ENABLE_PARALLEL)
    set(cgns_C_FILES ${cgns_C_FILES} pcgnslib.c)
  endif (CGNS_ENABLE_PARALLEL)
endif (CGNS_ENABLE_HDF5)

# All the Fortran files need by the cgns library (if enabled)
set(cgns_F_FILES
	cg_ftoc.c
	cgio_ftoc.c
	cgns_f.F90)

# Only build those files that are needed
if (CGNS_ENABLE_FORTRAN)
  if (FORTRAN_NAMING)
    set_property(SOURCE ${cgns_F_FILES}
      PROPERTY COMPILE_DEFINITIONS ${FORTRAN_NAMING})
  endif (FORTRAN_NAMING)
  set(cgns_FILES ${cgns_C_FILES} ${cgns_F_FILES})
else (CGNS_ENABLE_FORTRAN)
  set(cgns_FILES ${cgns_C_FILES})
endif (CGNS_ENABLE_FORTRAN)

# XXX(kitware): Handled by ParaView.
if (FALSE)
set(CGNS_BUILD_SHARED "ON" CACHE BOOL "Build a shared version of the library")
option(CGNS_USE_SHARED "Link programs to the CGNS shared library" "ON")

# Build a static version of the library
add_library(cgns_static STATIC ${cgns_FILES})

# Build a shared version of the library
if(CGNS_BUILD_SHARED)
  mark_as_advanced(CLEAR CGNS_USE_SHARED)
  add_library(cgns_shared SHARED ${cgns_FILES})
  # under windows we need to define BUILD_DLL
  # and link with the HDF5 libraries to build a dll
  if (WIN32 OR CYGWIN)
    add_definitions(-DBUILD_DLL)
    if (CGNS_ENABLE_HDF5 AND HDF5_LIBRARY)
      target_link_libraries(cgns_shared ${HDF5_LIBRARY})
      if(HDF5_NEED_ZLIB AND ZLIB_LIBRARY)
        target_link_libraries(cgns_shared ${ZLIB_LIBRARY})
      endif(HDF5_NEED_ZLIB AND ZLIB_LIBRARY)
      if(HDF5_NEED_SZIP AND SZIP_LIBRARY)
        target_link_libraries(cgns_shared ${SZIP_LIBRARY})
      endif(HDF5_NEED_SZIP AND SZIP_LIBRARY)
      if(HDF5_NEED_MPI AND MPI_LIBS)
        target_link_libraries(cgns_shared ${MPI_LIBS})
      endif(HDF5_NEED_MPI AND MPI_LIBS)
    endif (CGNS_ENABLE_HDF5 AND HDF5_LIBRARY)
  endif (WIN32 OR CYGWIN)
else(CGNS_BUILD_SHARED)
  set(CGNS_USE_SHARED "OFF")
  mark_as_advanced(FORCE CGNS_USE_SHARED)
endif(CGNS_BUILD_SHARED)

# Change the output name of the library to be libcgns
set_target_properties(cgns_static PROPERTIES OUTPUT_NAME cgns)
set_target_properties(cgns_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
if(CGNS_BUILD_SHARED)
  # for windows we need to change the name of the shared library
  # for both static and shared version to exist
  if (WIN32 OR CYGWIN)
    set_target_properties(cgns_shared PROPERTIES OUTPUT_NAME cgnsdll)
  else (WIN32 OR CYGWIN)
    set_target_properties(cgns_shared PROPERTIES OUTPUT_NAME cgns)
  endif (WIN32 OR CYGWIN)
  set_target_properties(cgns_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)
endif(CGNS_BUILD_SHARED)

# Set the version numbers
set_target_properties(cgns_static PROPERTIES VERSION "${CGNS_VERSION}")
set_target_properties(cgns_static PROPERTIES SOVERSION "${CGNS_VERSION}")

if(CGNS_BUILD_SHARED)
  set_target_properties(cgns_shared PROPERTIES VERSION "${CGNS_VERSION}")
  set_target_properties(cgns_shared PROPERTIES SOVERSION "${CGNS_VERSION}")
endif(CGNS_BUILD_SHARED)

# Set the install path of the static library
install(TARGETS cgns_static ARCHIVE DESTINATION lib)
# Set the install path of the shared library
if(CGNS_BUILD_SHARED)
  # for windows, need to install both cgnsdll.dll and cgnsdll.lib
  if (WIN32 OR CYGWIN)
    install(TARGETS cgns_shared ARCHIVE DESTINATION lib)
    install(TARGETS cgns_shared RUNTIME DESTINATION lib)
  else (WIN32 OR CYGWIN)
    install(TARGETS cgns_shared LIBRARY DESTINATION lib)
  endif (WIN32 OR CYGWIN)
endif(CGNS_BUILD_SHARED)
else ()
  vtk_add_library(vtkcgns ${cgns_FILES})
  target_link_libraries(vtkcgns ${vtkhdf5_LIBRARIES})
  set_target_properties(vtkcgns PROPERTIES DEFINE_SYMBOL BUILD_DLL)
endif ()

# XXX(kitware): Fix the include install directory.
if (NOT VTK_INSTALL_NO_DEVELOPMENT)
set(cgns_include_directory "${VTK_INSTALL_INCLUDE_DIR}/vtkcgns")

# Set the install path of the header files
install(FILES
	cgns_mangle.h
	cgnslib.h
	cgns_io.h
	#cgnswin_f.h
	${CMAKE_CURRENT_BINARY_DIR}/cgnsconfig.h
	${CMAKE_CURRENT_BINARY_DIR}/cgnstypes.h
	#${CMAKE_CURRENT_BINARY_DIR}/cgnstypes_f.h
	#${CMAKE_CURRENT_BINARY_DIR}/cgnstypes_f03.h
	#${CMAKE_CURRENT_BINARY_DIR}/cgnsBuild.defs
        DESTINATION ${cgns_include_directory})

if (CGNS_ENABLE_FORTRAN)
  install(FILES
          ${CMAKE_CURRENT_BINARY_DIR}/cgns.mod
          DESTINATION ${cgns_include_directory})
endif (CGNS_ENABLE_FORTRAN)

if (CGNS_ENABLE_PARALLEL)
  install(FILES	pcgnslib.h DESTINATION ${cgns_include_directory})
endif (CGNS_ENABLE_PARALLEL)

if (CGNS_ENABLE_LEGACY)
  install(FILES adf/ADF.h DESTINATION ${cgns_include_directory}/adf)
  if (CGNS_ENABLE_HDF5)
    install(FILES adfh/ADFH.h DESTINATION ${cgns_include_directory}/adfh)
  endif (CGNS_ENABLE_HDF5)
endif (CGNS_ENABLE_LEGACY)
endif()

#########
# Tools #
#########

# XXX(kitware): ParaView doesn't need this.
if (FALSE)
add_subdirectory(tools)

#########
# Tests #
#########

if (CGNS_ENABLE_TESTS)
  add_subdirectory(tests)
  add_subdirectory(Test_UserGuideCode/C_code)
  if (CGNS_ENABLE_PARALLEL)
    add_subdirectory(ptests)
  endif (CGNS_ENABLE_PARALLEL)
endif (CGNS_ENABLE_TESTS)

##############
# CGNS Tools #
##############


set(CGNS_BUILD_CGNSTOOLS "OFF" CACHE BOOL "Build the CGNSTools package")
if(CGNS_BUILD_CGNSTOOLS)
  add_subdirectory(cgnstools)
endif(CGNS_BUILD_CGNSTOOLS)
endif ()
