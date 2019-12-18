# Resolve the GenericIO external dependency
find_package(GenericIO REQUIRED)

# List of module sources
set(classes
  # Readers
  vtkACosmoReader
  vtkGenericIOUtilities
  vtkPCosmoReader
  vtkPGenericIOMultiBlockReader
  vtkPGenericIOMultiBlockWriter
  vtkPGenericIOReader
  vtkPMultiResolutionGenericIOReader

  # Filters
  vtkMinkowskiFilter
  vtkPANLHaloFinder
  vtkPANLSubhaloFinder
  vtkPLANLHaloFinder
  vtkPMergeConnected)

vtk_module_add_module(ParaView::VTKExtensionsCosmoTools
  CLASSES ${classes})
vtk_module_include(ParaView::VTKExtensionsCosmoTools
  PRIVATE
    ${GENERIC_IO_INCLUDE_DIR})
vtk_module_link(ParaView::VTKExtensionsCosmoTools
  PRIVATE
    ${GENERIC_IO_LIBRARIES})

paraview_add_server_manager_xmls(
  XMLS  ## CosmoTools Readers
        Resources/AdaptiveCosmoReader.xml
        Resources/CosmoReader.xml
        Resources/GenericIOReader.xml
        Resources/GenericIOWriter.xml
        Resources/MultiResolutionGenericIOReader.xml

        ## CosmoTools Filters
        Resources/ANLHaloFinder.xml
        Resources/ANLSubHaloFinder.xml
        Resources/LANLHaloFinder.xml
        Resources/MergeConnected.xml
        Resources/MinkowskiFilter.xml)
