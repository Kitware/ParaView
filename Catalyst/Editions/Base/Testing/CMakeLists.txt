cmake_minimum_required(VERSION 2.8)
include(CTest)
enable_testing()

add_test(NAME wavelet
  COMMAND ${PVPYTHON_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/wavelet.py)
