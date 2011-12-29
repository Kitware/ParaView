
set(MPL_binary "${CMAKE_CURRENT_BINARY_DIR}/MPL/")

if(APPLE)
  set(MPL_PERFIX
      "--home=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build")
  set(MPL_INSTALL_PURELIB
      "--install-purelib=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build/Utilities/VTKPythonWrapping/site-packages")
  set(MPL_INSTALL_PLATLIB
      "--install-platlib=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build/Utilities/VTKPythonWrapping/site-packages")
  set(MPL_INSTALL_SCRIPTS
      "--install-scripts=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build/bin")
  set(MPL_PREFIX_ARGS "${MPL_PERFIX} ${MPL_INSTALL_PURELIB} ${MPL_INSTALL_PLATLIB} ${MPL_INSTALL_SCRIPTS}")
endif()

configure_file(
  MPL_configure_step.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/MPL_configure_step.cmake @ONLY)
# to build matplotlib we also run a cmake -P script.
# the script will set LD_LIBRARY_PATH so that
# python can run after it is built on linux
configure_file(
  MPL_make_step.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/MPL_make_step.cmake @ONLY)

configure_file(
  MPL_install_step.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/MPL_install_step.cmake @ONLY)
  
set(MPL_PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different 
    "${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/MPLPatches/_png.cpp" "${MPL_binary}/src/_png.cpp")
set(MPL_CONFIGURE_COMMAND ${CMAKE_COMMAND}
    -DCONFIG_TYPE=${CMAKE_CFG_INTDIR} -P ${CMAKE_CURRENT_BINARY_DIR}/MPL_configure_step.cmake)
set(MPL_BUILD_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/MPL_make_step.cmake)
set(MPL_INSTALL_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/MPL_install_step.cmake)

# create an external project to download numpy,
# and configure and build it
ExternalProject_Add(MPL
  URL ${MPL_URL}/${MPL_GZ}
  URL_MD5 ${MPL_MD5}
  DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/MPL
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/MPL
  PATCH_COMMAND ${MPL_PATCH_COMMAND}
  CONFIGURE_COMMAND ${MPL_CONFIGURE_COMMAND}
  BUILD_COMMAND ${MPL_BUILD_COMMAND}
  UPDATE_COMMAND ""
  INSTALL_COMMAND ${MPL_INSTALL_COMMAND}
  DEPENDS
    ${MPL_dependencies}
  )
