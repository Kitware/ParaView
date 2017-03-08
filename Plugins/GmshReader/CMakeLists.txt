if (NOT ParaView_BINARY_DIR)
   cmake_minimum_required(VERSION 3.3)
   find_package(ParaView REQUIRED)
   include(${PARAVIEW_USE_FILE})
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Gmsh 3.0.7 REQUIRED)

include_directories(
  ${VTK_INCLUDE_DIRS}
  ${Gmsh_INCLUDE_DIRS}
  )

add_paraview_plugin(
  GmshReader "1.0"
  SERVER_MANAGER_XML
    GmshReaderServerManager.xml
  SERVER_MANAGER_SOURCES
    vtkGmshMetaReader.cxx
    vtkGmshReader.cxx
  )

target_link_libraries(GmshReader LINK_PRIVATE ${Gmsh_LIBRARIES})

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif()
