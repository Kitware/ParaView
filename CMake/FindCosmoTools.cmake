# - Try to find the GenericIO library
# Once done this will define
#
# COSMOTOOLS_FOUND -- boolean that indicates whether GenericIO was found
# COSMOTOOLS_INCLUDE_DIR -- the include path for GenericIO
# COSMOTOOLS_LIBRARIES -- the GenericIO libraries to link against

## Try to find include directory
find_path(COSMOTOOLS_INCLUDE_DIR
            NAMES CosmoToolsDefinitions.h
            PATHS /usr/include /usr/local/include
            )

## Try to find the GenericIO library
find_library(COSMOTOOLS_LIBRARIES
    NAMES cosmotools
    PATHS /usr/lib
    )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    CosmoTools DEFAULT_MSG COSMOTOOLS_INCLUDE_DIR COSMOTOOLS_LIBRARIES)
mark_as_advanced(COSMOTOOLS_INCLUDE_DIR)
mark_as_advanced(COSMOTOOLS_LIBRARIES)