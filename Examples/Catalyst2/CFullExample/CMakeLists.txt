cmake_minimum_required(VERSION 3.13)
project(CFullExampleV2 LANGUAGES C)

#------------------------------------------------------------------------------
add_executable(CFullExampleV2
  FEDataStructures.c
  FEDataStructures.h
  FEDriver.c)

# Since this example uses MPI, find and link against it.
find_package(MPI COMPONENTS C REQUIRED)
target_link_libraries(CFullExampleV2
  PRIVATE
    MPI::MPI_C)

#------------------------------------------------------------------------------
option(USE_CATALYST "Build example with Catalyst enabled" ON)
if (USE_CATALYST)
  find_package(catalyst REQUIRED
    PATHS "${ParaView_DIR}/catalyst")
  target_compile_definitions(CFullExampleV2
    PRIVATE
      USE_CATALYST=1)
  target_link_libraries(CFullExampleV2
    PRIVATE
      catalyst::catalyst)

  include(CTest)
  if (BUILD_TESTING)
    add_test(
      NAME CFullExampleV2::SimplePipeline
      COMMAND CFullExampleV2
              ${CMAKE_CURRENT_SOURCE_DIR}/catalyst_pipeline.py)
    set_tests_properties("CFullExampleV2::SimplePipeline"
      PROPERTIES
        FAIL_REGULAR_EXPRESSION "ERROR:;instances(s)? still around;ERR\\\|"
        SKIP_RETURN_CODE 125)
  endif()
endif()
