if (NOT USE_CATALYST)
  message(STATUS
    "Skipping Fortran90FullExample because `USE_CATALYST` is not set.")
  return ()
endif ()

add_executable(Fortran90FullExample
  FECxxAdaptor.cxx
  FEDriver.f90
  FEFortranAdaptor.f90)
target_link_libraries(Fortran90FullExample
  PRIVATE
    ParaView::PythonCatalyst
    MPI::MPI_Fortran)
set_property(TARGET Fortran90FullExample
  PROPERTY
    LINKER_LANGUAGE Fortran)

if (BUILD_TESTING)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/SampleScripts/coproc.py
    ${CMAKE_CURRENT_BINARY_DIR}/coproc.py
    COPYONLY)
  add_test(NAME Fortran90FullExampleTest COMMAND Fortran90FullExample)
endif()
