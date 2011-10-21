
project(hybrid)

include_directories(
  ${Nektar_SOURCE_DIR}/include
  ${MPI_INCLUDE_PATH}
  )

if(WIN32)
  add_definitions(-D_USE_MATH_DEFINES)
endif()

add_definitions(-DPARALLEL -DMETIS -DHOSURF -DZERONULLDIV -DSVVSYMM -DACCELERATOR -DCSGSLIB -DNDEBUG -DPBC_1D_LIN_ATREE)

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



add_library(hybrid STATIC ${hybrid_sources})
target_link_libraries(hybrid vec metis gs)
