
# The VRPN external project for ParaView
set(VRPN_source "${CMAKE_CURRENT_BINARY_DIR}/VRPN")
set(VRPN_binary "${CMAKE_CURRENT_BINARY_DIR}/VRPN-build")
set(VRPN_install "${CMAKE_CURRENT_BINARY_DIR}")

ExternalProject_Add(VRPN
  SOURCE_DIR ${VRPN_source}
  BINARY_DIR ${VRPN_binary}
  INSTALL_DIR ${VRPN_install}
  URL ${VRPN_URL}/${VRPN_GZ}
  URL_MD5 ${VRPN_MD5}
  CMAKE_CACHE_ARGS
    -DCMAKE_CXX_FLAGS:STRING=${pv_tpl_cxx_flags}
    -DCMAKE_C_FLAGS:STRING=${pv_tpl_c_flags}
    -DBUILD_SHARED_LIBS:BOOL=OFF
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
    ${pv_tpl_compiler_args}
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  )

if(WIN32)
  set(VRPN_LIBRARY ${VRPN_install}/lib/vrpn${_LINK_LIBRARY_SUFFIX})
else()
  set(VRPN_LIBRARY ${VRPN_install}/lib/libvrpn.a)
endif()

set(VRPN_INCLUDE_DIR ${VRPN_install}/include)
