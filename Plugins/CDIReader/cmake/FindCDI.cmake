# use this because CDI is installed in <prefix>/lib/cmake, but cmake does not look for that path
# (see https://cmake.org/cmake/help/latest/command/find_package.html#config-mode-search-procedure)
# This file may be removed once fixed in CDI as can be the CMAKE_MODULE_PATH leading to here.
find_package(CDI
  ${PACKAGE_FIND_VERSION}
  PATHS ${CDI_DIR}
  PATH_SUFFIXES lib/cmake lib64/cmake
  REQUIRED
  )
