cmake_minimum_required(VERSION 3.13)
project(CxxSteeringExample LANGUAGES C CXX)
enable_testing()

include (GNUInstallDirs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

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
add_executable(CxxSteeringExample
  FEDataStructures.cxx
  FEDataStructures.h
  FEDriver.cxx
  CatalystAdaptor.h
  catalyst_pipeline.py
  catalyst_steering_proxies.xml
  )
target_link_libraries(CxxSteeringExample
  PRIVATE
    MPI::MPI_C
    MPI::MPI_CXX)

#------------------------------------------------------------------------------
option(USE_CATALYST "Build example with Catalyst enabled" ON)
if (USE_CATALYST)
  find_package(catalyst REQUIRED
    PATHS "${ParaView_DIR}/catalyst")
  target_compile_definitions(CxxSteeringExample
    PRIVATE
      "PARAVIEW_IMPL_DIR=\"${ParaView_CATALYST_DIR}\""
      USE_CATALYST=1)
  target_link_libraries(CxxSteeringExample
    PRIVATE
      catalyst::catalyst)

  include(CTest)
  if (BUILD_TESTING)
    add_test(
      NAME CxxSteeringExample::SimplePipeline
      COMMAND CxxSteeringExample
              ${CMAKE_CURRENT_SOURCE_DIR}/catalyst_pipeline.py
              ${CMAKE_CURRENT_SOURCE_DIR}/catalyst_steering_proxies.xml)

    set(_vtk_fail_regex
        # CatalystAdaptor
        "Failed"
        # vtkLogger
        "(\n|^)ERROR: "
        "ERR\\|"
        # vtkDebugLeaks
        "instance(s)? still around")

    set_tests_properties("CxxSteeringExample::SimplePipeline"
      PROPERTIES
        FAIL_REGULAR_EXPRESSION "${_vtk_fail_regex}"
        PASS_REGULAR_EXPRESSION "executing catalyst_pipeline"
        SKIP_REGULAR_EXPRESSION "Python support not enabled"
        SKIP_RETURN_CODE 125)
  endif()
endif()
