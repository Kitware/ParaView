cmake_minimum_required(VERSION 2.8)
include(CTest)
enable_testing()

find_package(ParaViewCompileTools REQUIRED)
add_test(NAME WrapClientServer
  COMMAND vtkWrapClientServer --version)
