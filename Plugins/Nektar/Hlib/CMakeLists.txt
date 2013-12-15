enable_language(C CXX Fortran)

if(NOT MPI_INCLUDE_PATH)
  find_package(MPI REQUIRED)
endif()

include_directories(
  ${MPI_INCLUDE_PATH}
  ${Nektar_SOURCE_DIR}/include
  )

# check the endianness of the system and set the define macro accordingly.
# this behavior should be repeated in the full Nektar code.
include(TestBigEndian)
TEST_BIG_ENDIAN(CMAKE_WORDS_BIGENDIAN)

if(CMAKE_WORDS_BIGENDIAN)
  add_definitions(-DBTYPE="ieee_big_endian")
else()
  add_definitions(-DBTYPE="ieee_little_endian")
endif()

if(WIN32)
  add_definitions(-D_USE_MATH_DEFINES)
endif()

if(NOT WIN32)
# need some try compiles
  set(CMAKE_C_FLAGS "-fPIC -ffast-math -funroll-loops -fstrict-aliasing -w")
  set(CMAKE_CXX_FLAGS "-fPIC -ffast-math -funroll-loops -fstrict-aliasing -w")
  set(CMAKE_Fortran_FLAGS "-fPIC -ffast-math -funroll-loops -w")
endif()

add_definitions(-DPARALLEL -DMETIS -DHOSURF -DZERONULLDIV -DSVVSYMM -DACCELERATOR -DCSGSLIB -DNDEBUG -DPBC_1D_LIN_ATREE -DNOSCALAPACK)

#defines  =  HOSURF ZERONULLDIV SVVSYMM ACCELERATOR  CSGSLIB NDEBUG PBC_1D_LIN_ATREE #LE_VERT_OLD PARALLEL #VELINTERP #Lanczos #DUMPLEMODES

set(hybrid_sources
  Linux/manager.c
  src/Basis.C
  src/Boundary.C
  src/Coords.C
  src/Curvi.C
  src/Dbutils.C
  src/Deriv.C
  src/EigenMatrix.C
  src/Element.C
  src/Element_List.C
  src/Felisa.C
  src/Fieldfiles.C
  src/Geofac.C
  src/Gradient.C
  src/HOSurf.C
  src/Hex.C
  src/HOSurf.C
  src/InnerProd.C
  src/Integrate.C
  src/Interp.C
  src/Interp_point.C
  src/manager.y
  src/Matrix.C
  src/MatrixR.C
  src/Matrix_Stokes.C
  src/MC3D_nektar_mex.C
  src/Memory.C
  src/Misc.C
  src/Mrhs.C
  src/Nekvec.C
  src/Norms.C
  src/Nums.C
  src/Overlap.C
  src/OverlapFull.C
  src/Parallel.C
  src/Particle.C
  src/polylib.C
  src/Prism.C
  src/Pyr.C
  src/Quad.C
  src/Refine.C
  src/SMatrix.C
  src/Solve.C
  src/SolveR.C
  src/Solve_Stokes.C
  src/Solve_cg.C
  src/Tet.C
  src/Transforms.C
  src/tree.c
  src/Tri.C
  src/Utilities.C
  src/SMatrix.C
  src/Felisa.C
  src/FelisaF.F
  )


vtk_module_library(hybrid STATIC ${hybrid_sources})

set_target_properties(hybrid PROPERTIES LINKER_LANGUAGE C)