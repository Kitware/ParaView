cmake_minimum_required(VERSION 3.13)
project(CxxFullExampleV2 LANGUAGES C CXX)

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
    "Skipping example: ${PROJECT_NAME} requires MPI.")
  return ()
endif ()

#------------------------------------------------------------------------------
add_executable(CxxFullExampleV2
  FEDataStructures.cxx
  FEDataStructures.h
  FEDriver.cxx)
target_link_libraries(CxxFullExampleV2
  PRIVATE
    MPI::MPI_C
    MPI::MPI_CXX)

#------------------------------------------------------------------------------
option(USE_CATALYST "Build example with Catalyst enabled" ON)
if (USE_CATALYST)
  find_package(catalyst REQUIRED
    PATHS "${ParaView_DIR}/catalyst")
  target_compile_definitions(CxxFullExampleV2
    PRIVATE
      "PARAVIEW_IMPL_DIR=\"${ParaView_CATALYST_DIR}\""
      "PARAVIEW_EXTRA_PYTHONPATH=\"${CATALYST_PYTHONPATH}\""
      USE_CATALYST=1)
  target_link_libraries(CxxFullExampleV2
    PRIVATE
      catalyst::catalyst)

  include(CTest)
  if (BUILD_TESTING)
    add_test(
      NAME CxxFullExampleV2::SimplePipeline
      COMMAND CxxFullExampleV2
              ${CMAKE_CURRENT_SOURCE_DIR}/catalyst_pipeline.py)

    set(_vtk_fail_regex
      # CatalystAdaptor
      "Failed"
      # vtkLogger
      "(\n|^)ERROR: "
      "ERR\\|"
      # vtkDebugLeaks
      "instance(s)? still around")

    set_tests_properties("CxxFullExampleV2::SimplePipeline"
      PROPERTIES
        FAIL_REGULAR_EXPRESSION "${_vtk_fail_regex}"
        PASS_REGULAR_EXPRESSION "executing catalyst_pipeline"
        SKIP_REGULAR_EXPRESSION "Python support not enabled"
        SKIP_RETURN_CODE 125)
    
    add_test(
      NAME CxxFullExampleV2::SimplePipeline-WithRendering
      COMMAND CxxFullExampleV2
              ${CMAKE_CURRENT_SOURCE_DIR}/catalyst_pipeline_with_rendering.py)

    set_tests_properties("CxxFullExampleV2::SimplePipeline-WithRendering"
      PROPERTIES
        FAIL_REGULAR_EXPRESSION "${_vtk_fail_regex}"
        PASS_REGULAR_EXPRESSION "executing catalyst_pipeline"
        SKIP_REGULAR_EXPRESSION "Python support not enabled|Failed to create requested view"
        SKIP_RETURN_CODE 125)
  endif()
endif()
