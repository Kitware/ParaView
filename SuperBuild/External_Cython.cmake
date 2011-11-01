# The Cython external project

set(Cython_binary "${CMAKE_CURRENT_BINARY_DIR}/Cython/")

if(APPLE)
  set(Cython_PERFIX
      "--home=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build")
  set(Cython_INSTALL_PURELIB
      "--install-purelib=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build/Utilities/VTKPythonWrapping/site-packages")
  set(Cython_INSTALL_PLATLIB
      "--install-platlib=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build/Utilities/VTKPythonWrapping/site-packages")
  set(Cython_INSTALL_SCRIPTS
      "--install-scripts=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build/bin")
  set(Cython_PREFIX_ARGS "${Cython_PERFIX} ${Cython_INSTALL_PURELIB} ${Cython_INSTALL_PLATLIB} ${Cython_INSTALL_SCRIPTS}")
endif()

# to configure cython we run a cmake -P script
# the script will create a site.cfg file
# then run python setup.py config to verify setup
configure_file(
  Cython_configure_step.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/Cython_configure_step.cmake @ONLY)
# to build cython we also run a cmake -P script.
# the script will set LD_LIBRARY_PATH so that
# python can run after it is built on linux
configure_file(
  Cython_make_step.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/Cython_make_step.cmake @ONLY)

configure_file(
  Cython_install_step.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/Cython_install_step.cmake @ONLY)

set(Cython_CONFIGURE_COMMAND ${CMAKE_COMMAND}
    -DCONFIG_TYPE=${CMAKE_CFG_INTDIR} -P ${CMAKE_CURRENT_BINARY_DIR}/Cython_configure_step.cmake)
set(Cython_BUILD_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/Cython_make_step.cmake)
set(Cython_INSTALL_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/Cython_install_step.cmake)

# create an external project to download cython,
# and configure and build it
ExternalProject_Add(Cython
  URL ${Cython_URL}/${Cython_GZ}
  URL_MD5 ${Cython_MD5}
  DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/Cython
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/Cython
  CONFIGURE_COMMAND ${Cython_CONFIGURE_COMMAND}
  BUILD_COMMAND ${Cython_BUILD_COMMAND}
  UPDATE_COMMAND ""
  INSTALL_COMMAND ${Cython_INSTALL_COMMAND}
  DEPENDS
    ${Cython_dependencies}
  )
