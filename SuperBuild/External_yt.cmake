# The yt external project

set(yt_binary "${CMAKE_CURRENT_BINARY_DIR}/yt/")

if(APPLE)
  set(yt_PERFIX
      "--home=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build")
  set(yt_INSTALL_PURELIB
      "--install-purelib=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build/Utilities/VTKPythonWrapping/site-packages")
  set(yt_INSTALL_PLATLIB
      "--install-platlib=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build/Utilities/VTKPythonWrapping/site-packages")
  set(yt_INSTALL_SCRIPTS
      "--install-scripts=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build/bin")
  set(yt_PREFIX_ARGS "${yt_PERFIX} ${yt_INSTALL_PURELIB} ${yt_INSTALL_PLATLIB} ${yt_INSTALL_SCRIPTS}")
endif()

# to configure yt we run a cmake -P script
# the script will create a site.cfg file
# then run python setup.py config to verify setup
configure_file(
  yt_configure_step.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/yt_configure_step.cmake @ONLY)
# to build yt we also run a cmake -P script.
# the script will set LD_LIBRARY_PATH so that
# python can run after it is built on linux
configure_file(
  yt_make_step.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/yt_make_step.cmake @ONLY)

configure_file(
  yt_install_step.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/yt_install_step.cmake @ONLY)

set(yt_CONFIGURE_COMMAND ${CMAKE_COMMAND}
    -DCONFIG_TYPE=${CMAKE_CFG_INTDIR} -P ${CMAKE_CURRENT_BINARY_DIR}/yt_configure_step.cmake)
set(yt_BUILD_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/yt_make_step.cmake)
set(yt_INSTALL_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/yt_install_step.cmake)

# create an external project to download yt,
# and configure and build it
ExternalProject_Add(yt
  URL ${yt_URL}/${yt_GZ}
  URL_MD5 ${yt_MD5}
  DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/yt
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/yt
  CONFIGURE_COMMAND ${yt_CONFIGURE_COMMAND}
  BUILD_COMMAND ${yt_BUILD_COMMAND}
  UPDATE_COMMAND ""
  INSTALL_COMMAND ${yt_INSTALL_COMMAND}
  DEPENDS
    ${yt_dependencies}
  )
