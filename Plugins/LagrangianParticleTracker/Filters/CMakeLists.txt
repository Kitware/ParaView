set(classes
  vtkLagrangianHelperBase
  vtkLagrangianSeedHelper
  vtkLagrangianSurfaceHelper)

vtk_module_add_module(LagrangianParticleTracker::vtkLagrangianParticleTracker
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  LagrangianParticleTracker.xml)
