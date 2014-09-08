cmake_minimum_required(VERSION 2.8.8)
project(CatalystCxxFullExample)


find_package(ParaView 4.1 REQUIRED COMPONENTS vtkPVPythonCatalyst)
include("${PARAVIEW_USE_FILE}")
#include("${PARAVIEW_CMAKE_DIR}/ParaViewTestInstall.cmake")

set(PARAVIEW_INSTALL_DIR "${VTK_INSTALL_PREFIX}")
set(DOLFIN_EXAMPLE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(DOLFIN_EXAMPLE_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(DOLFIN_EXAMPLE_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/run-original.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/run-original.sh)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/run-catalyst-step1.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/run-catalyst-step1.sh)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/run-catalyst-step2.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/run-catalyst-step2.sh)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/run-catalyst-step3.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/run-catalyst-step3.sh)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/run-catalyst-step4.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/run-catalyst-step4.sh)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/run-catalyst-step6.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/run-catalyst-step6.sh)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/simulation-catalyst-step1.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/simulation-catalyst-step1.py)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/simulation-catalyst-step2.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/simulation-catalyst-step2.py)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/simulation-catalyst-step3.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/simulation-catalyst-step3.py)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/simulation-catalyst-step4.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/simulation-catalyst-step4.py)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/simulation-catalyst-step6.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/simulation-catalyst-step6.py)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/simulation.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/simulation.py)

