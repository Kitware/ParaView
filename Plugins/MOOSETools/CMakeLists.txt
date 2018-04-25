include_directories(
  ${VTK_INCLUDE_DIRS})

# create a paraview plugin containing server manager xml and the server
# manager classes to build
# this plugin can be loaded on the server side

ADD_PARAVIEW_PLUGIN(MooseXfemClip "1.0"
  DOCUMENTATION_DIR "${CMAKE_CURRENT_SOURCE_DIR}/doc"
  REQUIRED_ON_SERVER
  SERVER_MANAGER_XML MooseXfemClip.xml
  SERVER_MANAGER_SOURCES vtkMooseXfemClip.cxx)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif()
