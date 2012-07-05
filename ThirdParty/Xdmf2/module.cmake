set (__dependencies)
if (PARAVIEW_USE_MPI)
  list(APPEND __dependencies vtkParallelMPI)
endif ()

vtk_module(vtkxdmf2
  GROUPS
    ParaView
  DEPENDS
    vtkRenderingOpenGL
    vtkRenderingFreeTypeOpenGL
    vtkIOCore
    vtkhdf5
    vtklibxml2
    ${__dependencies}
  EXCLUDE_FROM_WRAPPING
 )
