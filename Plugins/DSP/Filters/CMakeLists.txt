set(MODULE_CLASSES
  vtkMeanPowerSpectralDensity
  vtkMergeReduceTableBlocks
  vtkProjectSpectrumMagnitude
  vtkSoundQuantitiesCalculator
  vtkSpectrogramFilter
)

set(MODULE_HEADERS
  vtkAccousticUtilities.h
)

vtk_module_add_module(DigitalSignalProcessing::DSPFiltersPlugin
  FORCE_STATIC  # build static library, to avoid confusion when loading
  CLASSES       ${MODULE_CLASSES}
  HEADERS       ${MODULE_HEADERS}
)

paraview_add_server_manager_xmls(
  XMLS  DSPFilters.xml)
