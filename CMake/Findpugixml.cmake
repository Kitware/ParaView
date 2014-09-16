find_library(PUGIXML_LIBRARIES
    NAMES pugixml
)
find_path(PUGIXML_INCLUDE_DIRS
    NAMES pugixml.hpp
    PATH_SUFFIXES pugixml
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(pugixml DEFAULT_MSG
    PUGIXML_LIBRARIES
    PUGIXML_INCLUDE_DIRS
)
