find_package(PDAL QUIET REQUIRED)

# Search for e57 PDAL plugin built with xerces.
find_library(PDAL_plugin_e57_LIBRARY
  NAMES
    pdal_plugin_reader_e57 libpdal_plugin_reader_e57
  HINTS "${PDAL_LIBRARY_DIRS}"
  DOC "PDAL plugin e57")
mark_as_advanced(PDAL_plugin_e57_LIBRARY)

if (NOT PDAL_plugin_e57_LIBRARY)
  message(FATAL_ERROR
    "E57 PDAL lib is required to build the plugin.")
endif ()

paraview_add_plugin(E57PDALReader
  VERSION "1.0"
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  SERVER_MANAGER_XML ReaderE57PDAL.xml)
