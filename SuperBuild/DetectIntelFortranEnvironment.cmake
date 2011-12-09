# for VS IDE Intel Fortran we have to figure out the
# implicit link path for the fortran run time using
# a try-compile
IF(CMAKE_Fortran_COMPILER MATCHES "ifort" AND "${CMAKE_GENERATOR}" MATCHES "Visual Studio")
  SET(_desc "Determine Intel Fortran Compiler Environment")
  MESSAGE(STATUS "${_desc}")
  # Build a sample project which reports symbols.
  TRY_COMPILE(IFORT_LIB_PATH_COMPILED
    ${CMAKE_BINARY_DIR}/CMakeFiles/IntelVSEnvironment
    ${ParaViewSuperBuild_SOURCE_DIR}/IntelVSEnvironment
    IntelFortranEnvironment
    CMAKE_FLAGS
    "-DCMAKE_Fortran_FLAGS:STRING=${CMAKE_Fortran_FLAGS}"
    OUTPUT_VARIABLE _output)
  FILE(WRITE
    "${CMAKE_BINARY_DIR}/CMakeFiles/IntelVSEnvironment/output.txt"
    "${_output}")
  INCLUDE(${CMAKE_BINARY_DIR}/CMakeFiles/IntelVSEnvironment/output.cmake OPTIONAL)
  SET(_desc "Determine Intel Fortran Compiler Environment -- done")
  MESSAGE(STATUS "${_desc}")
ENDIF()