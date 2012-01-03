
project(nektarobj)


if(NOT WIN32)
# need some try compiles
  set(CMAKE_CXX_FLAGS "-fPIC")
endif()

# ParaView
if(NOT ParaView_SOURCE_DIR)
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})
endif()

add_library(nektarobj STATIC nektarObject.C)
