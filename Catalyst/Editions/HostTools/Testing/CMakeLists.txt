cmake_minimum_required(VERSION 3.3)
include(CTest)
enable_testing()

find_package(ParaViewCompileTools REQUIRED)
add_test(NAME WrapClientServer
  COMMAND vtkWrapClientServer --version)
