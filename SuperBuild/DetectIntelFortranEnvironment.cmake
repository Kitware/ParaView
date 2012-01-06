# for VS IDE Intel Fortran we have to figure out the
# implicit link path for the fortran run time using
# a try-compile
if(NOT intel_ifort_path AND CMAKE_Fortran_COMPILER MATCHES "ifort" AND "${CMAKE_GENERATOR}" MATCHES "Visual Studio")
  set(_desc "Determine Intel Fortran Compiler Environment")
  message(STATUS "${_desc}")
  # Build a sample project which reports symbols.
  try_compile(IFORT_ENVIRONMENT_COMPILED
    ${CMAKE_BINARY_DIR}/CMakeFiles/IntelVSEnvironment
    ${ParaViewSuperBuild_SOURCE_DIR}/IntelVSEnvironment
    IntelFortranEnvironment
    CMAKE_FLAGS
    "-DCMAKE_Fortran_FLAGS:STRING=${CMAKE_Fortran_FLAGS}"
    OUTPUT_VARIABLE _output)
  file(WRITE
    "${CMAKE_BINARY_DIR}/CMakeFiles/IntelVSEnvironment/output.txt"
    "${_output}")
  include(${CMAKE_BINARY_DIR}/CMakeFiles/IntelVSEnvironment/output.cmake OPTIONAL)
  set(_desc "Determine Intel Fortran Compiler Environment -- done")
  message(STATUS "${_desc}")
endif()