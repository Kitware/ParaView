## Improve vtkParticleTracerBase filters

The ParticleTracer/ParticlePath/StreakLine filters which are subclasses of vtkParticleTracerBase have the following
improvements:

1) Multithreaded using vtkSMPTools. Multithreading is used when there is only one MPI process,
   and the number of particles is greater than 100.
3) Now have _InterpolatorType_ which can either use a cell locator (default) or a point locator for interpolation.
4) Instead of the _StaticMesh_ flag, it now has the _MeshOverTime_ flag which has the following values:
   1) **Different**: The mesh is different over time.
   2) **Static**: The mesh is the same over time.
   3) **Linear Transformation**: The mesh is different over time, but it is a linear transformation of the first
      time-step's mesh.
      1) For cell locators, this flag internally makes use of the new vtkLinearTransformCellLocator. This way the
         locator is only built once in the first time-step.
      2) For point locators, this flag internally re-uses the same cell links, but rebuilds the point locator since
         there is no vtkLinearTransformPointLocator yet.
   4) **Same Topology**: The mesh is different over time, but it preserves the same topology (same number of
      points/cells, same connectivity).
      1) For cell locators, this is equivalent to _MeshOverTime_ == **Different**.
      2) For point locators, this flag internally re-uses the same cell links.
