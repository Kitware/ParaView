set(classes
  vtkCDIReader)

set(sources
  cdilib.c)

set(private_headers
  cdi.h)

set(CMAKE_C_STANDARD 99)
vtk_module_add_module(CDIReader::vtkCDIReader
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers})

vtk_module_definitions(CDIReader::vtkCDIReader
  PRIVATE
    # Please leave these definitions in here and do not change them. We use the
    # cdi library to read both, netCDF2 and netCDF4 data.
    HAVE_LIBNETCDF
    HAVE_NETCDF2
    HAVE_NETCDF4)

paraview_add_server_manager_xmls(
  XMLS  CDIReader.xml)
