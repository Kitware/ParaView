cmake_minimum_required(VERSION 3.13)
project(Fortran90FullExample)

enable_language(Fortran)

include (GNUInstallDirs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

# Since this example uses MPI, find and link against it.
find_package(MPI COMPONENTS Fortran)
if (NOT MPI_FOUND)
  message(STATUS
    "Skipping example: ${PROJECT_NAME} requires MPI.")
  return ()
endif ()

#------------------------------------------------------------------------------
add_executable(Fortran90FullExampleV2
  FEDriver.f90
  FEFortranAdaptor.F90)

target_link_libraries(Fortran90FullExampleV2
  PRIVATE
    MPI::MPI_Fortran)

set_property(TARGET Fortran90FullExampleV2
  PROPERTY
    LINKER_LANGUAGE Fortran)

set_property(TARGET Fortran90FullExampleV2
  PROPERTY
    POSITION_INDEPENDENT_CODE "${BUILD_SHARED_LIBS}")

#------------------------------------------------------------------------------
option(USE_CATALYST "Build example with Catalyst enabled" ON)
if (USE_CATALYST)
  find_package(catalyst REQUIRED
    PATHS "${ParaView_DIR}/catalyst")
  
  target_compile_definitions(Fortran90FullExampleV2
    PRIVATE
      "PARAVIEW_IMPL_DIR=\"${ParaView_CATALYST_DIR}\""
      USE_CATALYST=1)
  target_link_libraries(Fortran90FullExampleV2
    PRIVATE
      catalyst::catalyst_fortran)
  include(CTest)
  if (BUILD_TESTING)
    add_test(
      NAME Fortran90FullExampleV2::SimplePipeline
      COMMAND Fortran90FullExampleV2
              ${CMAKE_CURRENT_SOURCE_DIR}/catalyst_pipeline.py)

    set(_vtk_fail_regex
      # CatalystAdaptor
      "Failed"
      # vtkLogger
      "(\n|^)ERROR: "
      "ERR\\|"
      # vtkDebugLeaks
      "instance(s)? still around")

    set_tests_properties("Fortran90FullExampleV2::SimplePipeline"
      PROPERTIES
        FAIL_REGULAR_EXPRESSION "${_vtk_fail_regex}"
        PASS_REGULAR_EXPRESSION "executing catalyst_pipeline"
        SKIP_REGULAR_EXPRESSION "Python support not enabled"
        SKIP_RETURN_CODE 125)
  endif()
endif()
