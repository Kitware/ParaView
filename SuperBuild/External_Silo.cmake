
# The Silo external project for ParaView
set(Silo_source "${CMAKE_CURRENT_BINARY_DIR}/Silo")
set(Silo_binary "${CMAKE_CURRENT_BINARY_DIR}/Silo-build")
set(Silo_install "${CMAKE_CURRENT_BINARY_DIR}/Silo-install")

# If Windows we use CMake otherwise ./configure
if(WIN32)

  #set(Silo_PATCH_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/Silo_patch_step.cmake)
  set(Silo_CONFIGURE_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/Silo_configure_step.cmake)
  set(Silo_BUILD_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/Silo_make_step.cmake)
  set(Silo_INSTALL_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/Silo_install_step.cmake)

  ExternalProject_Add(Silo
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SOURCE_DIR ${Silo_source}
    BINARY_DIR ${Silo_build}
    INSTALL_DIR ${Silo_install}
    URL ${SILO_URL}/${SILO_GZ}
    URL_MD5 ${SILO_MD5}
    BUILD_IN_SOURCE ${Silo_BUILD_IN_SOURCE}
    PATCH_COMMAND <SOURCE_DIR>/SiloWindows/copysilo.bat
    BUILD_COMMAND ${Silo_BUILD_COMMAND}
    UPDATE_COMMAND ""
    INSTALL_COMMAND ${Silo_INSTALL_COMMAND}
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
