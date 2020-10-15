set(CMAKE_PREFIX_PATH          "/usr/lib64/openmpi" CACHE STRING "")
set(PARAVIEW_BUILD_SHARED_LIBS OFF                  CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_common.cmake")
