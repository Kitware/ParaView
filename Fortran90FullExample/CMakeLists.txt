cmake_minimum_required(VERSION 2.6)

PROJECT(CoProcessingExample CXX Fortran)

find_package(ParaView 3.98 REQUIRED COMPONENTS vtkPVPythonCatalyst)

INCLUDE(${PARAVIEW_USE_FILE})
if(NOT PARAVIEW_USE_MPI)
  message(SEND_ERROR "ParaView must be built with MPI enabled")
endif()
if(NOT MPI_Fortran_LIBRARIES)
  find_package(MPI)
endif()

ADD_EXECUTABLE(FEDriver FEDriver.f90 FEFortranAdaptor.f90 FECxxAdaptor.cxx)
TARGET_LINK_LIBRARIES(FEDriver vtkPVPythonCatalyst ${MPI_Fortran_LIBRARIES})

file(COPY SampleScripts/coproc.py DESTINATION ${CMAKE_BINARY_DIR})
