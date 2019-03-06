set(classes
  vtkGMVReader)

# gmvread.c is included by vtkGMVReader.cxx.

set(private_headers
  gmvrayread.h
  gmvread.h)

vtk_module_add_module(GMVReader::vtkGMVReader
  CLASSES ${classes}
  PRIVATE_HEADERS ${private_headers})
vtk_module_definitions(GMVReader::vtkGMVReader
  PRIVATE
    # Disable deprecation warnings for C and STL functions for MSVC.
    "$<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_DEPRECATE>")

vtk_module_compile_options(GMVReader::vtkGMVReader
  PRIVATE
    # Disable "ignoring return value of ..." warnings from included gmvread.c
    "$<$<CXX_COMPILER_ID:GNU>:-Wno-unused-result>")

paraview_add_server_manager_xmls(
  XMLS  GMVReaderSM.xml)
