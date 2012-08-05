project(EyeDomeLighting)
cmake_minimum_required(VERSION 2.8.8)

if (NOT ParaView_SOURCE_DIR)
  find_package(ParaView)
  include("${PARAVIEW_USE_FILE}")
else()
  # we're building from within ParaView Source.
endif()

#add_subdirectory(src)
#add_subdirectory(ParaViewPlugin)
