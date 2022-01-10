if (PARAVIEW_USE_MPI)
  set(classes
    vtkPCGNSWriter) 

  vtk_object_factory_declare(
    BASE     vtkCGNSWriter
    OVERRIDE vtkPCGNSWriter)

  vtk_object_factory_configure(
    SOURCE_FILE vtk_object_factory_source
    HEADER_FILE vtk_object_factory_header
    EXPORT_MACRO "VTKPVVTKEXTENSIONSPCGNSWRITER_EXPORT")

  vtk_module_add_module(ParaView::VTKExtensionsPCGNSWriter
    CLASSES ${classes}
    SOURCES ${vtk_object_factory_source}
    PRIVATE_HEADERS ${vtk_object_factory_header})

  if(CGNS_ENABLE_PARALLEL)
    vtk_module_definitions(ParaView::VTKExtensionsPCGNSWriter 
      PRIVATE CGNS_HAS_PARALLEL
    )
  endif()
  paraview_add_server_manager_xmls(
    XMLS resources/PCGNSWriter.xml)
endif(PARAVIEW_USE_MPI)
