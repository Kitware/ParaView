set(CMAKE_C_CLANG_TIDY "clang-tidy-cache" "--header-filter=$ENV{CI_PROJECT_DIR}/([A-OQ-SW-Z]|Plugins/([A-BD-FH-KM-Za-oq-z]|CAVEInteraction|CDIReader/Reader/vtk*.h|CFSReader|ContourLabelPlugin|GenericIOReader/Readers/[^/]*.h|GeodesicMeasurement/Filters/vtk*.h|GeographicalMap|GmshIO|GmshReader|GMVReader/Reader/vtk*.h|LagrangianParticleTracker|LegacyExodusReader|LegacyExodusWriter|LookingGlass|pvblot|pvNVIDIAIndex/(kernel_programs|src|Testing))|Testing|Utilities/[A-UW-Z]|Utilities/Versioning|VTKExtensions)" CACHE FILEPATH "")
set(CMAKE_CXX_CLANG_TIDY "clang-tidy-cache" "--header-filter=$ENV{CI_PROJECT_DIR}/([A-OQ-SW-Z]|Plugins/([A-BD-FH-KM-Za-oq-z]|CAVEInteraction|CDIReader/Reader/vtk*.h|CFSReader|ContourLabelPlugin|GenericIOReader/Readers/[^/]*.h|GeodesicMeasurement/Filters/vtk*.h|GeographicalMap|GmshIO|GmshReader|GMVReader/Reader/vtk*.h|LagrangianParticleTracker|LegacyExodusReader|LegacyExodusWriter|LookingGlass|pvblot|pvNVIDIAIndex/(kernel_programs|src|Testing))|Testing|Utilities/[A-UW-Z]|Utilities/Versioning|VTKExtensions)" CACHE FILEPATH "")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "")

set(PARAVIEW_ENABLE_VISITBRIDGE ON CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora42_shared_asserts_mpi_python_qt.cmake")
