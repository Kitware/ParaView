cmake_minimum_required(VERSION 3.3)
project(CamAdaptor CXX)

find_package(ParaView REQUIRED COMPONENTS vtkPVPythonCatalyst)

INCLUDE(${PARAVIEW_USE_FILE})
if(NOT PARAVIEW_USE_MPI)
  message(SEND_ERROR "ParaView must be built with MPI enabled")
endif()

configure_file(cesm1_2_2.patch.in cesm1_2_2.patch @ONLY)

ADD_LIBRARY(CamAdaptor
  Grid.cxx
  fv_create_data.cxx
  se_create_data.cxx)
TARGET_LINK_LIBRARIES(CamAdaptor vtkPVPythonCatalyst)

if (CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wshadow -Woverloaded-virtual -Wno-deprecated")
endif()
