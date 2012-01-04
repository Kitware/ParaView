# The yt external project

set(distribute_binary "${CMAKE_CURRENT_BINARY_DIR}/distribute/")

if(APPLE)
  set(distribute_PERFIX
      "--home=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build")
  set(distribute_INSTALL_PURELIB
      "--install-purelib=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build/Utilities/VTKPythonWrapping/site-packages")
  set(distribute_INSTALL_PLATLIB
      "--install-platlib=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build/Utilities/VTKPythonWrapping/site-packages")
  set(distribute_INSTALL_SCRIPTS
      "--install-scripts=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build/bin")
  set(distribute_PREFIX_ARGS "${distribute_PERFIX} ${distribute_INSTALL_PURELIB} ${distribute_INSTALL_PLATLIB} ${distribute_INSTALL_SCRIPTS}")
endif()

configure_file(
  distribute_make_step.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/distribute_make_step.cmake @ONLY)

configure_file(
  distribute_install_step.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/distribute_install_step.cmake @ONLY)

set(distribute_BUILD_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/distribute_make_step.cmake)
set(distribute_INSTALL_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/distribute_install_step.cmake)

ExternalProject_Add(distribute
  URL ${distribute_URL}/${distribute_GZ}
  URL_MD5 ${distribute_MD5}
  DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/distribute
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/distribute
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ${distribute_BUILD_COMMAND}
  INSTALL_COMMAND ${distribute_INSTALL_COMMAND}
  DEPENDS ${distribute_dependencies}
  )
