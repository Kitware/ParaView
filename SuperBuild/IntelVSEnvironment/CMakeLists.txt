cmake_minimum_required (VERSION 2.8)
project(IntelFortranEnvironment Fortran)
add_custom_command(
  OUTPUT output.cmake
  COMMAND ${CMAKE_COMMAND} -P ${IntelFortranEnvironment_SOURCE_DIR}/detect_environment.cmake
  )
add_library(FortranLib hello.f output.cmake)
