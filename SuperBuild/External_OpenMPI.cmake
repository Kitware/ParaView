
# The OpenMPI external project for ParaView
set(OpenMPI_source "${CMAKE_CURRENT_BINARY_DIR}/OpenMPI")
set(OpenMPI_binary "${CMAKE_CURRENT_BINARY_DIR}/OpenMPI-build")
set(OpenMPI_install "${CMAKE_CURRENT_BINARY_DIR}/OpenMPI-install")


ExternalProject_Add(OpenMPI
  DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
  SOURCE_DIR ${OpenMPI_source}
  BINARY_DIR ${OpenMPI_build}
  INSTALL_DIR ${OpenMPI_install}
  URL ${OPENMPI_URL}/${OPENMPI_GZ}
  URL_MD5 ${OPENMPI_MD5}
  CMAKE_CACHE_ARGS
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_CFG_INTDIR}
    ${pv_tpl_compiler_args}
    ${OpenMPI_EXTRA_ARGS}
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  )

set(OpenMPI_DIR "${OpenMPI_binary}" CACHE PATH
  "OpenMPI binary directory" FORCE)
mark_as_advanced(OpenMPI_DIR)
