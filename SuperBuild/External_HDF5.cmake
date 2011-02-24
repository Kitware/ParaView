
# The HDF5 external project for ParaView
set(hdf5_source "${CMAKE_CURRENT_BINARY_DIR}/hdf5")
set(hdf5_install "${CMAKE_CURRENT_BINARY_DIR}/hdf5-install")

# If Windows we use CMake otherwise ./configure
if(WIN32)

  set(hdf5_binary "${CMAKE_CURRENT_BINARY_DIR}/hdf5-build")

  ExternalProject_Add(HDF5
  URL ${HDF5_URL}/${HDF5_GZ}
  URL_MD5 ${HDF5_MD5}
  UPDATE_COMMAND ""
  SOURCE_DIR ${hdf5_source}
  BINARY_DIR ${hdf5_binary}
  INSTALL_DIR ${hdf5_install}
  CMAKE_CACHE_ARGS
    -DCMAKE_CXX_FLAGS:STRING=${pv_tpl_cxx_flags}
    -DCMAKE_C_FLAGS:STRING=${pv_tpl_c_flags}
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_CFG_INTDIR}
    ${pv_tpl_compiler_args}
    -DZLIB_INCLUDE_DIR:STRING=${ZLIB_INCLUDE_DIR}
    -DZLIB_LIBRARY:STRING=${ZLIB_LIBRARY}
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  DEPENDS ${HDF5_dependencies}
  )

else()

  ExternalProject_Add(HDF5
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SOURCE_DIR ${hdf5_source}
    INSTALL_DIR ${hdf5_install}
    URL ${HDF5_URL}/${HDF5_GZ}
    URL_MD5 ${HDF5_MD5}
    BUILD_IN_SOURCE 1
    PATCH_COMMAND ""
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
  )

endif()
