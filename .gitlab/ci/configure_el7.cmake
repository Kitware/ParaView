# el7 docker image does not have catalyst installed
set(PARAVIEW_ENABLE_CATALYST OFF CACHE BOOL "")

# SPDX generation requires a more recent python
set(PARAVIEW_GENERATE_SPDX OFF CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora_common.cmake")
