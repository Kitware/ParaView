# The LAPACK external project for Titan

set(lapack_source "${CMAKE_CURRENT_BINARY_DIR}/LAPACK")
set(lapack_binary "${CMAKE_CURRENT_BINARY_DIR}/LAPACK-build")
set(NUMPY_LAPACK_binary ${lapack_binary})

ExternalProject_Add(LAPACK
  DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
  SOURCE_DIR ${lapack_source}
  BINARY_DIR ${lapack_binary}
  URL ${LAPACK_URL}/${LAPACK_GZ}
  URL_MD5 ${LAPACK_MD5}
  CMAKE_ARGS
    -DCMAKE_CXX_FLAGS:STRING=${pv_tpl_cxx_flags}
    -DCMAKE_C_FLAGS:STRING=${pv_tpl_c_flags}
    -DBUILD_SHARED_LIBS:BOOL=ON
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
    ${LAPACK_EXTRA_ARGS}
  INSTALL_COMMAND ""
  )
list(APPEND trilinos_depends LAPACK)
