option(BUILD_FORTRAN_COPROCESSING_ADAPTORS "Build a coprocessing adaptor that uses Fortran" OFF)
mark_as_advanced(BUILD_FORTRAN_COPROCESSING_ADAPTORS)
if(BUILD_FORTRAN_COPROCESSING_ADAPTORS)
  add_subdirectory(FortranAdaptors)
endif(BUILD_FORTRAN_COPROCESSING_ADAPTORS)

