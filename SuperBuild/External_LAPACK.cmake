# The LAPACK external project for Titan

set(lapack_source "${CMAKE_CURRENT_BINARY_DIR}/LAPACK")
set(lapack_binary "${CMAKE_CURRENT_BINARY_DIR}/LAPACK-build")

ExternalProject_Add(LAPACK
  DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
  SOURCE_DIR ${lapack_source}
  BINARY_DIR ${lapack_binary}
  URL ${lapack_file}
  URL_MD5 ${lapack_md5}
  CMAKE_ARGS
    -DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}
    -DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}
    -DBUILD_SHARED_LIBS:BOOL=ON
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
    ${LAPACK_EXTRA_ARGS}
  INSTALL_COMMAND ""
  )
list(APPEND trilinos_depends LAPACK)
set(trilinos_blas_args
  -DTPL_BLAS_LIBRARIES:PATH=${lapack_binary}/SRC/liblapack.so
  -DTPL_LAPACK_LIBRARIES:PATH=${lapack_binary}/BLAS/SRC/libblas.so)
