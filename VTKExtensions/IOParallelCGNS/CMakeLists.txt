set(classes
  vtkPCGNSWriter)

vtk_object_factory_declare(
  BASE     vtkCGNSWriter
  OVERRIDE vtkPCGNSWriter)

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKPVVTKEXTENSIONSIOPARALLELCGNSWRITER_EXPORT")

vtk_module_add_module(ParaView::VTKExtensionsIOParallelCGNSWriter
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header})

if(CGNS_ENABLE_PARALLEL)
  vtk_module_definitions(ParaView::VTKExtensionsIOParallelCGNSWriter
    PRIVATE CGNS_HAS_PARALLEL
  )
endif()
