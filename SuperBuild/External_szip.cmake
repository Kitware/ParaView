
# The SZIP external project for ParaView
set(SZIP_source "${CMAKE_CURRENT_BINARY_DIR}/SZIP")
set(SZIP_install "${CMAKE_CURRENT_BINARY_DIR}")

set(SZIP_binary "${CMAKE_CURRENT_BINARY_DIR}/SZIP-build")

ExternalProject_Add(szip
  URL ${SZIP_URL}/${SZIP_GZ}
  URL_MD5 ${SZIP_MD5}
  UPDATE_COMMAND ""
  SOURCE_DIR ${SZIP_source}
  BINARY_DIR ${SZIP_binary}
  INSTALL_DIR ${SZIP_install}
  CMAKE_CACHE_ARGS
    -DBUILD_SHARED_LIBS:BOOL=ON
    -DBUILD_TESTING:BOOL=OFF
    -DCMAKE_CXX_FLAGS:STRING=${pv_tpl_cxx_flags}
    -DCMAKE_C_FLAGS:STRING=${pv_tpl_c_flags}
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_CFG_INTDIR}
    ${pv_tpl_compiler_args}
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  DEPENDS ${SZIP_dependencies}
  )


if(WIN32)
  set(SZIP_INCLUDE_DIR ${SZIP_install}/include)
  set(SZIP_LIBRARY ${SZIP_install}/lib/szipdll${_LINK_LIBRARY_SUFFIX})
else()
  set(SZIP_INCLUDE_DIR ${SZIP_install}/include)
  set(SZIP_LIBRARY ${SZIP_install}/lib/libszip${_LINK_LIBRARY_SUFFIX})
endif()