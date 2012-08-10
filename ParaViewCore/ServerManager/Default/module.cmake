unset(_vtk_modules)
# this file defines all ParaView needed modules and defines "_vtk_modules"
include(VTKModules)

vtk_module(vtkPVServerManagerDefault
  DEPENDS
    vtkPVServerImplementationDefault
    vtkPVServerManagerRendering
    ${_vtk_modules}

  TEST_DEPENDS
    vtkPVServerManagerApplication
)
unset(_vtk_modules)
