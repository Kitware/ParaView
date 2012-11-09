# brings in the list of modules to enable by default for the full ParaView
# application.
include(VTKModules)

vtk_module(vtkPVServerManagerApplication
  DEPENDS
    # When creating a "custom" application, simply change this to
    # depend on the appropriate module.
    vtkPVServerManagerCore
  COMPILE_DEPENDS
    vtkUtilitiesProcessXML
    # this enables the necesary modules.
    ${PARAVIEW_DEFAULT_VTK_MODULES}
  TEST_LABELS
    PARAVIEW
)
