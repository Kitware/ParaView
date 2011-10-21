# The Numpy external project

set(NUMPY_binary "${CMAKE_CURRENT_BINARY_DIR}/NUMPY/")

if(APPLE)
  set(NUMPY_PERFIX
      "--home=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build")
  set(NUMPY_INSTALL_PURELIB
      "--install-purelib=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build/Utilities/VTKPythonWrapping/site-packages")
  set(NUMPY_INSTALL_PLATLIB
      "--install-platlib=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build/Utilities/VTKPythonWrapping/site-packages")
  set(NUMPY_INSTALL_SCRIPTS
      "--install-scripts=${CMAKE_CURRENT_BINARY_DIR}/ParaView-build/bin")
  set(NUMPY_PREFIX_ARGS "${NUMPY_PERFIX} ${NUMPY_INSTALL_PURELIB} ${NUMPY_INSTALL_PLATLIB} ${NUMPY_INSTALL_SCRIPTS}")
endif()

if(CMAKE_Fortran_COMPILER AND NOT WIN32)
  get_filename_component(fortran_compiler ${CMAKE_Fortran_COMPILER} NAME_WE)
  if("${fortran_compiler}" STREQUAL "gfortran")
    set(fcompiler_arg --fcompiler=gnu95)
  elseif("${fortran_compiler}" STREQUAL "g77")
    set(fcompiler_arg --fcompiler=gnu)
  elseif("${fortran_compiler}" STREQUAL "ifort")
    set(fcompiler_arg --fcompiler=intel)
  endif()
endif()

# to configure numpy we run a cmake -P script
# the script will create a site.cfg file
# then run python setup.py config to verify setup
configure_file(
  NUMPY_configure_step.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/NUMPY_configure_step.cmake @ONLY)
# to build numpy we also run a cmake -P script.
# the script will set LD_LIBRARY_PATH so that
# python can run after it is built on linux
configure_file(
  NUMPY_make_step.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/NUMPY_make_step.cmake @ONLY)

configure_file(
  NUMPY_install_step.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/NUMPY_install_step.cmake @ONLY)

set(NUMPY_CONFIGURE_COMMAND ${CMAKE_COMMAND}
    -DCONFIG_TYPE=${CMAKE_CFG_INTDIR} -P ${CMAKE_CURRENT_BINARY_DIR}/NUMPY_configure_step.cmake)
set(NUMPY_BUILD_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/NUMPY_make_step.cmake)
set(NUMPY_INSTALL_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/NUMPY_install_step.cmake)



# create an external project to download numpy,
# and configure and build it
ExternalProject_Add(NUMPY
  URL ${NUMPY_URL}/${NUMPY_GZ}
  URL_MD5 ${NUMPY_MD5}
  DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/NUMPY
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/NUMPY
  CONFIGURE_COMMAND ${NUMPY_CONFIGURE_COMMAND}
  BUILD_COMMAND ${NUMPY_BUILD_COMMAND}
  UPDATE_COMMAND ""
  INSTALL_COMMAND ${NUMPY_INSTALL_COMMAND}
  DEPENDS
    ${NUMPY_dependencies}
  )
