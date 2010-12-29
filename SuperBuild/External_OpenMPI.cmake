
# The OpenMPI external project for ParaView
set(OpenMPI_source "${CMAKE_CURRENT_BINARY_DIR}/OpenMPI")
set(OpenMPI_binary "${CMAKE_CURRENT_BINARY_DIR}/OpenMPI-build")
set(OpenMPI_install "${CMAKE_CURRENT_BINARY_DIR}/OpenMPI-install")

# If Windows we use CMake otherwise ./configure
if(WIN32)

  ExternalProject_Add(OpenMPI
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SOURCE_DIR ${OpenMPI_source}
    BINARY_DIR ${OpenMPI_build}
    INSTALL_DIR ${OpenMPI_install}
    URL ${OPENMPI_URL}/${OPENMPI_GZ}
    URL_MD5 ${OPENMPI_MD5}
    CMAKE_CACHE_ARGS
      -DCMAKE_CXX_FLAGS:STRING=${pv_tpl_cxx_flags}
      -DCMAKE_C_FLAGS:STRING=${pv_tpl_c_flags}
      -DCMAKE_BUILD_TYPE:STRING=${CMAKE_CFG_INTDIR}
      ${pv_tpl_compiler_args}
      ${OpenMPI_EXTRA_ARGS}
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  )

else()
  ExternalProject_Add(OpenMPI
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SOURCE_DIR ${OpenMPI_source}
    BINARY_DIR ${OpenMPI_build}
    INSTALL_DIR ${OpenMPI_install}
    URL ${OPENMPI_URL}/${OPENMPI_GZ}
    URL_MD5 ${OPENMPI_MD5}
    BUILD_IN_SOURCE 1
    PATCH_COMMAND ""
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
  )

endif()

set(OpenMPI_DIR "${OpenMPI_binary}" CACHE PATH "OpenMPI binary directory" FORCE)
mark_as_advanced(OpenMPI_DIR)
