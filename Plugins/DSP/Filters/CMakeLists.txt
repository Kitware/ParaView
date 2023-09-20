set(MODULE_CLASSES
  vtkBandFiltering
  vtkDSPTableFFT
  vtkMeanPowerSpectralDensity
  vtkMergeReduceTables
  vtkMultiDimensionBrowser
  vtkProjectSpectrumMagnitude
  vtkSoundQuantitiesCalculator
  vtkSpectrogramFilter
  vtkTemporalMultiplexing
)

set(MODULE_HEADERS
  vtkAccousticUtilities.h
)

vtk_module_add_module(DigitalSignalProcessing::DSPFiltersPlugin
  FORCE_STATIC  # build static library, to avoid confusion when loading
  CLASSES         ${MODULE_CLASSES}
  PRIVATE_CLASSES ${MODULE_PRIVATE_CLASSES}
  HEADERS         ${MODULE_HEADERS}
)

paraview_add_server_manager_xmls(
  XMLS  DSPFilters.xml)
