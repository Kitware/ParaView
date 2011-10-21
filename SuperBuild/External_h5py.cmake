# The h5py external project

set(h5py_binary "${CMAKE_CURRENT_BINARY_DIR}/h5py/")

if(APPLE)
  set(h5py_PERFIX
      "--home=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build")
  set(h5py_INSTALL_PURELIB
      "--install-purelib=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build/Utilities/VTKPythonWrapping/site-packages")
  set(h5py_INSTALL_PLATLIB
      "--install-platlib=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build/Utilities/VTKPythonWrapping/site-packages")
  set(h5py_INSTALL_SCRIPTS
      "--install-scripts=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build/bin")
  set(h5py_PREFIX_ARGS "${h5py_PERFIX} ${h5py_INSTALL_PURELIB} ${h5py_INSTALL_PLATLIB} ${h5py_INSTALL_SCRIPTS}")
endif()

# to configure h5py we run a cmake -P script
# the script will create a site.cfg file
# then run python setup.py config to verify setup
configure_file(
  h5py_configure_step.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/h5py_configure_step.cmake @ONLY)
# to build h5py we also run a cmake -P script.
# the script will set LD_LIBRARY_PATH so that
# python can run after it is built on linux
configure_file(
  h5py_make_step.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/h5py_make_step.cmake @ONLY)

configure_file(
  h5py_install_step.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/h5py_install_step.cmake @ONLY)

set(h5py_PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different 
    "${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/h5pyPatches/setup.py.${h5py_VERSION}" "${h5py_binary}/setup.py")
set(h5py_CONFIGURE_COMMAND ${CMAKE_COMMAND}
    -DCONFIG_TYPE=${CMAKE_CFG_INTDIR} -P ${CMAKE_CURRENT_BINARY_DIR}/h5py_configure_step.cmake)
set(h5py_BUILD_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/h5py_make_step.cmake)
set(h5py_INSTALL_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/h5py_install_step.cmake)

# create an external project to download h5py,
# and configure and build it
ExternalProject_Add(h5py
  URL ${h5py_URL}/${h5py_GZ}
  URL_MD5 ${h5py_MD5}
  DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/h5py
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/h5py
  PATCH_COMMAND ${h5py_PATCH_COMMAND}
  CONFIGURE_COMMAND ${h5py_CONFIGURE_COMMAND}
  BUILD_COMMAND ${h5py_BUILD_COMMAND}
  UPDATE_COMMAND ""
  INSTALL_COMMAND ${h5py_INSTALL_COMMAND}
  DEPENDS
    ${h5py_dependencies}
  )

if(WIN32)
  ExternalProject_Add_Step(h5py PatchUnistd
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/h5pyPatches/unistd.h" "${h5py_binary}/win_include/unistd.h"
    DEPENDEES patch
    DEPENDERS configure
    )
endif()

ExternalProject_Add_Step(h5py RemoveEggInfo
  COMMAND ${CMAKE_COMMAND} -E remove_directory "${h5py_binary}/h5py.egg-info"
  DEPENDEES patch
  DEPENDERS configure
  )