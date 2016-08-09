cmake_minimum_required(VERSION 3.3)
include(CTest)
enable_testing()

find_package(ParaView REQUIRED)
include("${PARAVIEW_USE_FILE}")
include_directories(${PARAVIEW_INCLUDE_DIRS})

file(GLOB dirs */)

foreach (dir ${dirs})
  if (IS_DIRECTORY "${dir}")
    add_subdirectory("${dir}")
  endif ()
endforeach ()
