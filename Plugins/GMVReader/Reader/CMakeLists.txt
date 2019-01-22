set(classes
  vtkGMVReader)

# gmvread.c is included by vtkGMVReader.cxx.

set(private_headers
  gmvrayread.h
  gmvread.h)

option(PARAVIEW_PLUGIN_GMVReader_SKIP_DATARANGE_CALCULATIONS
  "Calculate minimum and maximum of data arrays in RequestInformation calls. A
  feature inherited from AVSucdReader, but it seems this information is never
  queried by ParaView."
  OFF)
mark_as_advanced(PARAVIEW_PLUGIN_GMVReader_SKIP_DATARANGE_CALCULATIONS)
set(skip_datarange_calculations 0)
if (PARAVIEW_PLUGIN_GMVReader_SKIP_DATARANGE_CALCULATIONS)
  set(skip_datarange_calculations 1)
endif ()

vtk_module_add_module(GMVReader::vtkGMVReader
  CLASSES ${classes}
  PRIVATE_HEADERS ${private_headers})
vtk_module_define(GMVReader::vtkGMVReader
  PRIVATE
    "PARAVIEW_PLUGIN_GMVReader_SKIP_DATARANGE_CALCULATIONS=${skip_datarange_calculations}"
    # Disable deprecation warnings for C and STL functions for MSVC.
    "$<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_DEPRECATE>")

vtk_module_compile_options(GMVReader::vtkGMVReader
  PRIVATE
    # Disable "ignoring return value of ..." warnings from included gmvread.c
    "$<$<CXX_COMPILER_ID:GNU>:-Wno-unused-result>")

paraview_add_server_manager_xmls(
  XMLS  GMVReaderSM.xml)
