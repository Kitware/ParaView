# The Fortran adaptor API assumes that a python script is being
# used to drive the coprocessing pipelines. Because of this
# we only make it available if ParaView has been built with
# Python. There is no strict need for this though.
if (PARAVIEW_ENABLE_PYTHON)
  option(BUILD_FORTRAN_COPROCESSING_ADAPTORS
    "Build a coprocessing adaptor that can be called from Fortran or C" OFF)
  mark_as_advanced(BUILD_FORTRAN_COPROCESSING_ADAPTORS)
  if(BUILD_FORTRAN_COPROCESSING_ADAPTORS)
    add_subdirectory(FortranAdaptors)
  endif(BUILD_FORTRAN_COPROCESSING_ADAPTORS)
endif()

option(BUILD_PARTICLE_COPROCESSING_ADAPTORS "Build a coprocessing adaptor that
renders particle systems" OFF)
mark_as_advanced(BUILD_PARTICLE_COPROCESSING_ADAPTORS)
if(BUILD_PARTICLE_COPROCESSING_ADAPTORS)
  add_subdirectory(ParticleAdaptor)
endif(BUILD_PARTICLE_COPROCESSING_ADAPTORS)

