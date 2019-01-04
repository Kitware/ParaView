set(classes
  vtkPVRandomPointsStreamingSource
  vtkStreamingParticlesPriorityQueue
  vtkStreamingParticlesRepresentation)

vtk_module_add_module(StreamingParticles::vtkStreamingParticles
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  StreamingParticles.xml)
