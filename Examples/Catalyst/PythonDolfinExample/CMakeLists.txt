cmake_minimum_required(VERSION 3.3)
project(CatalystCxxFullExample)

find_package(ParaView 4.2 REQUIRED COMPONENTS vtkPVPythonCatalyst)
include("${PARAVIEW_USE_FILE}")

if(VTK_INSTALL_PREFIX)
  # we are in the install tree of ParaView that includes development files.
  # the ParaViewConfig.cmake file is located at
  # <installdir>lib/cmake/paraview-<majorversion>.<minorversion>
  set(PV_MAIN_DIR ${VTK_INSTALL_PREFIX})
  set(PV_MAIN_SUBDIR "paraview-${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}")
else()
  set(PV_MAIN_DIR ${ParaView_DIR})
endif()
# The location of the ParaView and VTK libraries are set to an environment
# variable that gets set in the run-catalyst-step*.sh scripts.
set(LDLIBRARYPATH "${PV_MAIN_DIR}/lib/${PV_MAIN_SUBDIR}")

set(DOLFIN_EXAMPLE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(DOLFIN_EXAMPLE_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(DOLFIN_EXAMPLE_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/run-original.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/run-original.sh @ONLY)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/run-catalyst-step1.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/run-catalyst-step1.sh @ONLY)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/run-catalyst-step2.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/run-catalyst-step2.sh @ONLY)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/run-catalyst-step3.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/run-catalyst-step3.sh @ONLY)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/run-catalyst-step4.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/run-catalyst-step4.sh @ONLY)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/run-catalyst-step6.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/run-catalyst-step6.sh @ONLY)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/simulation-env.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/simulation-env.py @ONLY)
