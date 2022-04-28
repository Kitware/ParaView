# Plugin infrastructure added in ADIOS 2.8.0
# 2.8.3 has a fix needed for catalyst live
find_package(ADIOS2 2.8.3 REQUIRED
  COMPONENTS CXX C
  OPTIONAL_COMPONENTS MPI Fortran)
find_package(catalyst 2.0 REQUIRED)

add_library(ParaViewADIOSInSituEngine
  ParaViewFidesEngine.cxx)

include(GenerateExportHeader)
generate_export_header(ParaViewADIOSInSituEngine)

target_link_libraries(ParaViewADIOSInSituEngine
  PUBLIC
    adios2::adios2
    adios2::cxx11
    adios2::core
  PRIVATE
    catalyst::catalyst
)

target_include_directories(ParaViewADIOSInSituEngine PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>")

install(TARGETS ParaViewADIOSInSituEngine DESTINATION lib/ADIOS
  EXPORT ParaViewADIOSInSituEngineTargets)
install(EXPORT ParaViewADIOSInSituEngineTargets
  FILE ParaViewADIOSInSituEngineTargets.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ADIOS)
