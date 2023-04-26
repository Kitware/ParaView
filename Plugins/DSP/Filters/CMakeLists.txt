set(classes
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

set(headers
  vtkAccousticUtilities.h
)

vtk_module_add_module(DigitalSignalProcessing::DSPFiltersPlugin
  FORCE_STATIC  # build static library, to avoid confusion when loading
  CLASSES         ${classes}
  HEADERS         ${headers}
)

paraview_add_server_manager_xmls(
  XMLS  DSPFilters.xml)
