cmake_minimum_required(VERSION 2.6)

PROJECT(CoProcessingExample CXX Fortran)

find_package(ParaView 3.98 REQUIRED COMPONENTS vtkPVPythonCatalyst)

INCLUDE(${PARAVIEW_USE_FILE})
ADD_EXECUTABLE(FEDriver FEDriver.f90 FEFortranAdaptor.f90 FECxxAdaptor.cxx)
TARGET_LINK_LIBRARIES(FEDriver vtkPVPythonCatalyst)
