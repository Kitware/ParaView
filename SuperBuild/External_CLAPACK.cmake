
# The CLAPACK external project for ParaView
set(CLAPACK_source "${CMAKE_CURRENT_BINARY_DIR}/CLAPACK")
set(CLAPACK_binary "${CMAKE_CURRENT_BINARY_DIR}/CLAPACK-build")
set(NUMPY_LAPACK_binary ${CLAPACK_binary})

#
# To fix compilation problem: relocation R_X86_64_32 against `a local symbol' can not be
# used when making a shared object; recompile with -fPIC
# See http://www.cmake.org/pipermail/cmake/2007-May/014350.html
#
if(UNIX AND CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  set(pv_tpl_c_flags_LAPACK "-fPIC ${pv_tpl_c_flags}")
endif()

configure_file(${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/CLAPACK_install_step.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/CLAPACK_install_step.cmake
    @ONLY)

set(CLAPACK_INSTALL_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/CLAPACK_install_step.cmake)

ExternalProject_Add(CLAPACK
  DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
  SOURCE_DIR ${CLAPACK_source}
  BINARY_DIR ${CLAPACK_binary}
  URL ${CLAPACK_URL}/${CLAPACK_GZ}
  URL_MD5 ${CLAPACK_MD5}
  CMAKE_CACHE_ARGS
    -DCMAKE_C_FLAGS:STRING=${pv_tpl_c_flags_LAPACK}
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_CFG_INTDIR}
    -DBUILD_SHARED_LIBS:BOOL=OFF
    ${pv_tpl_compiler_args}
    ${CLAPACK_EXTRA_ARGS}
  INSTALL_COMMAND ${CLAPACK_INSTALL_COMMAND}
  )

if(WIN32)
  set(BLAS_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/lib/blas${_LINK_LIBRARY_SUFFIX}")
  set(LAPACK_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/lib/lapack${_LINK_LIBRARY_SUFFIX}")
  set(F2C_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/lib/libf2c${_LINK_LIBRARY_SUFFIX}")
else()
  set(BLAS_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/lib/libblas.a")
  set(LAPACK_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/lib/liblapack.a")
  set(F2C_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/lib/libf2c.a")
endif()
