
# The OpenMPI external project for ParaView
set(Silo_source "${CMAKE_CURRENT_BINARY_DIR}/OpenMPI")
set(Silo_binary "${CMAKE_CURRENT_BINARY_DIR}/OpenMPI-build")
set(Silo_install "${CMAKE_CURRENT_BINARY_DIR}/OpenMPI-install")

# If Windows we use CMake otherwise ./configure
if(WIN32)

  ExternalProject_Add(Silo
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SOURCE_DIR ${Silo_source}
    BINARY_DIR ${Silo_build}
    INSTALL_DIR ${Silo_install}
    URL ${SILO_URL}/${SILO_GZ}
    URL_MD5 ${SILO_MD5}
    CMAKE_CACHE_ARGS
      -DCMAKE_CXX_FLAGS:STRING=${pv_tpl_cxx_flags}
      -DCMAKE_C_FLAGS:STRING=${pv_tpl_c_flags}
      -DCMAKE_BUILD_TYPE:STRING=${CMAKE_CFG_INTDIR}
      ${pv_tpl_compiler_args}
      ${Silo_EXTRA_ARGS}
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  )

else()

  ExternalProject_Add(Silo
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SOURCE_DIR ${Silo_source}
    BINARY_DIR ${Silo_build}
    INSTALL_DIR ${Silo_install}
    URL ${OPENMPI_URL}/${OPENMPI_GZ}
    URL_MD5 ${OPENMPI_MD5}
    BUILD_IN_SOURCE 1
    PATCH_COMMAND ""
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
  )

endif()

set(Silo_DIR "${Silo_binary}" CACHE PATH "Silo binary directory" FORCE)
mark_as_advanced(Silo_DIR)
