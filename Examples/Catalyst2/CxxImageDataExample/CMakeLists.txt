cmake_minimum_required(VERSION 3.13)
project(CxxImageDataExampleV2 LANGUAGES C CXX)

#------------------------------------------------------------------------------
# since we use C++11 in this example.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Since this example uses MPI, find and link against it.
find_package(MPI COMPONENTS C CXX)
if (NOT MPI_FOUND)
  message(STATUS
    "Skipping example: ${CMAKE_PROJECT_NAME} requires MPI.")
  return ()
endif ()

#------------------------------------------------------------------------------
add_executable(CxxImageDataExampleV2
  FEDataStructures.cxx
  FEDataStructures.h
  FEDriver.cxx)
target_link_libraries(CxxImageDataExampleV2
  PRIVATE
    MPI::MPI_C
    MPI::MPI_CXX)

#------------------------------------------------------------------------------
option(USE_CATALYST "Build example with Catalyst enabled" ON)
if (USE_CATALYST)
  find_package(catalyst REQUIRED
    PATHS "${ParaView_DIR}/catalyst")
  target_compile_definitions(CxxImageDataExampleV2
    PRIVATE
      USE_CATALYST=1)
  target_link_libraries(CxxImageDataExampleV2
    PRIVATE
      catalyst::catalyst)

  # when using Conduit's C++ API to ease creation of conduit nodes, you have
  # explicitly link against `catalyst::conduit` target.
  target_link_libraries(CxxImageDataExampleV2
    PRIVATE
      catalyst::conduit)

  include(CTest)
  if (BUILD_TESTING)
    add_test(
      NAME CxxImageDataExampleV2::SimplePipeline
      COMMAND CxxImageDataExampleV2
              ${CMAKE_CURRENT_SOURCE_DIR}/catalyst_pipeline.py)
    add_test(
      NAME CxxImageDataExampleV2::IOPipeline
      COMMAND CxxImageDataExampleV2
              --output "${CMAKE_CURRENT_BINARY_DIR}/Testing/Temporary/dataset-%04ts.vtpd")

    set_tests_properties(
        "CxxImageDataExampleV2::SimplePipeline"
        "CxxImageDataExampleV2::IOPipeline"
      PROPERTIES
        FAIL_REGULAR_EXPRESSION "ERROR:;instances(s)? still around;ERR\\\|"
        SKIP_RETURN_CODE 125)
  endif()
endif()
