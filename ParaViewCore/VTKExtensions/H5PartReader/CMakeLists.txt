## Thanks to John Biddiscombe of
## CSCS - Swiss National Supercomputing Centre for creating and contributing
## this plugin.

set(classes
  vtkH5PartReader)

set(sources
  # TODO: Move H5Part to ThirdParty/
  H5Part/src/H5Block.c
  H5Part/src/H5Part.c)
set_source_files_properties(${sources}
  PROPERTIES
    LANGUAGE CXX)

vtk_module_add_module(ParaView::VTKExtensionsH5PartReader
  CLASSES ${classes}
  SOURCES ${sources})
if (PARAVIEW_USE_MPI)
  vtk_module_definitions(ParaView::VTKExtensionsH5PartReader
    PRIVATE
      H5PART_HAS_MPI)
endif ()
vtk_module_definitions(ParaView::VTKExtensionsH5PartReader
  PRIVATE
    H5_USE_16_API)
vtk_module_include(ParaView::VTKExtensionsH5PartReader
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/H5Part/src")

paraview_add_server_manager_xmls(
  XMLS  H5PartServerManager.xml)
