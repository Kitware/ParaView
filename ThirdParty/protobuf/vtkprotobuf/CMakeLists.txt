project(vtkprotobuf)

find_package(Threads)
set(THREAD_LINK_LIB "")
if (Threads_FOUND)
  set(HAVE_PTHREAD 1)
  set(THREAD_LINK_LIB "${CMAKE_THREAD_LIBS_INIT}")
endif ()

# Source code for protobuf and its test cases
add_subdirectory(src)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/protobuf-function.cmake"
  "${CMAKE_BINARY_DIR}/${_vtk_build_CMAKE_DESTINATION}/protobuf-function.cmake"
  COPYONLY)
