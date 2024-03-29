cmake_minimum_required(VERSION 3.10)
project(CatalystExamples C CXX)

#------------------------------------------------------------------------------
# since we use C++11 in this example.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CMakeDependentOption)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../VTK/CMake")
cmake_dependent_option(USE_CATALYST "Link the simulator with Catalyst" ON
  "NOT WIN32" OFF)
if (USE_CATALYST)
  find_package(ParaView REQUIRED)

  if (NOT TARGET ParaView::PythonCatalyst)
    message(STATUS
      "Skipping example: ${PROJECT_NAME} requires ParaView to be built "
      "with Catalyst and Python support enabled. Please rebuild ParaView (or "
      "point to a different build of ParaView) with PARAVIEW_USE_PYTHON set "
      "to TRUE")
    return ()
  endif()

  if (NOT PARAVIEW_USE_MPI)
    message(STATUS
      "Skipping example: ${PROJECT_NAME} requires ParaView to be built "
      "with MPI support enabled. Please rebuild ParaView (or point to a "
      "different build of ParaView) with PARAVIEW_USE_MPI set to TRUE")
    return ()
  endif ()

  # FIXME: This should really be fixed to instead be done per-target.
  add_definitions(-DUSE_CATALYST)
else ()
  find_package(MPI COMPONENTS C)
  if (NOT MPI_FOUND)
    message(STATUS
      "Skipping example: ${PROJECT_NAME} requires MPI support, but none "
      "was found.")
    return ()
  endif ()
endif ()

option(BUILD_TESTING "Build Testing" OFF)
# Setup testing.
if (BUILD_TESTING)
  include(CTest)
endif()

# Static builds take a lot of memory to link. Avoid stampeding them all at
# once.
if (NOT BUILD_SHARED_LIBS)
  set_property(GLOBAL APPEND PROPERTY
    JOB_POOLS serialize=1)
  set(CMAKE_JOB_POOL_LINK serialize)
endif ()

add_subdirectory(CFullExample)
add_subdirectory(CFullExample2)
add_subdirectory(CxxFullExample)
add_subdirectory(CxxGhostCellsExample)
# FIXME: HTG API has changed since this was written.
#add_subdirectory(CxxHyperTreeGridExample)
add_subdirectory(CxxImageDataExample)
add_subdirectory(CxxMappedDataArrayExample)
add_subdirectory(CxxMultiChannelInputExample)
add_subdirectory(CxxMultiPieceExample)
add_subdirectory(CxxNonOverlappingAMRExample)
add_subdirectory(CxxOverlappingAMRExample)
if (TARGET ParaView::RemotingMisc)
  add_subdirectory(CxxPVSMPipelineExample)
  add_subdirectory(CxxParticlePathExample)
endif ()
add_subdirectory(CxxSOADataArrayExample)
if (TARGET ParaView::VTKExtensionsFiltersGeneral)
  add_subdirectory(CxxVTKPipelineExample)
endif()
add_subdirectory(MPISubCommunicatorExample)
add_subdirectory(PythonDolfinExample)
#add_subdirectory(PythonFullExample)
add_subdirectory(TemporalCacheExample)

include(CheckLanguage)
check_language(Fortran)
set(fortran_works OFF)
if (CMAKE_Fortran_COMPILER)
  enable_language(Fortran)
  set(fortran_works ON)
endif ()
option(BUILD_FORTRAN_EXAMPLES "Build Fortran Catalyst Examples" "${fortran_works}")
if (BUILD_FORTRAN_EXAMPLES)
  # Theoretically, CheckFortran should not be needed, but
  # enable_language(OPTIONAL) fails with Ninja generator.
  list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/../../CMake")
  include(CheckFortran)
  check_fortran_support()
  if (CMAKE_Fortran_COMPILER)
    enable_language(Fortran OPTIONAL)
  endif ()
  if (CMAKE_Fortran_COMPILER_WORKS)
    find_package(MPI REQUIRED COMPONENTS Fortran)

    add_subdirectory(Fortran90FullExample)
    add_subdirectory(FortranPoissonSolver)
  endif ()
endif ()
