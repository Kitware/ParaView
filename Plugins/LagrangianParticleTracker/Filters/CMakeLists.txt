set(classes
  vtkLagrangianHelperBase
  vtkLagrangianSeedHelper
  vtkLagrangianSurfaceHelper)

vtk_module_add_module(LagrangianParticleTracker::vtkLagrangianParticleTracker
  CLASSES ${classes})
