# The LAPACK external project

set(lapack_source "${CMAKE_CURRENT_BINARY_DIR}/LAPACK")
set(lapack_binary "${CMAKE_CURRENT_BINARY_DIR}/LAPACK-build")
set(lapack_install "${CMAKE_CURRENT_BINARY_DIR}")
set(NUMPY_LAPACK_binary ${lapack_binary})

if(WIN32)
  if(CMAKE_Fortran_COMPILER_ID MATCHES "Intel")
    include(DetectIntelFortranEnvironment)
    list(APPEND LAPACK_EXTRA_ARGS 
      -DCMAKE_Fortran_COMPILER:FILEPATH=${intel_ifort_path}/ifort.exe
    )
  endif()
else()
  set(LAPACK_EXTRA_ARGS 
    -DBUILD_STATIC_LIBS:BOOL=OFF
    -DBUILD_SHARED_LIBS:BOOL=ON
    )
endif()

if(UNIX AND CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  set(CMAKE_Fortran_FLAGS -fPIC)
endif()

ExternalProject_Add(LAPACK
  DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
  SOURCE_DIR ${lapack_source}
  BINARY_DIR ${lapack_binary}
  INSTALL_DIR ${lapack_install}
  URL ${LAPACK_URL}/${LAPACK_GZ}
  URL_MD5 ${LAPACK_MD5}
  CMAKE_CACHE_ARGS
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_CFG_INTDIR}
    -DBUILD_TESTING:BOOL=OFF
    ${LAPACK_EXTRA_ARGS}
  CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  )

if(WIN32)
  set(BLAS_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/lib/blas${_LINK_LIBRARY_SUFFIX}")
  set(LAPACK_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/lib/lapack${_LINK_LIBRARY_SUFFIX}")
else()
  set(BLAS_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/lib/libblas${_LINK_LIBRARY_SUFFIX}")
  set(LAPACK_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/lib/liblapack${_LINK_LIBRARY_SUFFIX}")
endif()
