option(BUILD_FORTRAN_COPROCESSING_ADAPTORS "Build a coprocessing adaptor that uses Fortran" OFF)
mark_as_advanced(BUILD_FORTRAN_COPROCESSING_ADAPTORS)
if(BUILD_FORTRAN_COPROCESSING_ADAPTORS)
  add_subdirectory(FortranAdaptors)
endif(BUILD_FORTRAN_COPROCESSING_ADAPTORS)

option(BUILD_PYTHON_COPROCESSING_ADAPTOR "Build a coprocessing adaptor that uses python" OFF)
mark_as_advanced(BUILD_PYTHON_COPROCESSING_ADAPTOR)
if(BUILD_PYTHON_COPROCESSING_ADAPTOR)
  add_subdirectory(PythonAdaptor)
endif(BUILD_PYTHON_COPROCESSING_ADAPTOR)

