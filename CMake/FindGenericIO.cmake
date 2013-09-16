# - Try to find the GenericIO library
# Once done this will define
#
# GENERIC_IO_FOUND -- boolean that indicates whether GenericIO was found
# GENERIC_IO_INCLUDE_DIR -- the include path for GenericIO
# GENERIC_IO_LIBRARIES -- the GenericIO libraries to link against

## Try to find include directory
find_path(GENERIC_IO_INCLUDE_DIR
            NAMES GenericIO.h
            PATHS /usr/include /usr/local/include
            )

## Try to find the GenericIO library
find_library(GENERIC_IO_LIBRARIES
    NAMES GenericIO
    PATHS /usr/lib
    )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    GenericIO DEFAULT_MSG GENERIC_IO_INCLUDE_DIR GENERIC_IO_LIBRARIES)
mark_as_advanced(GENERIC_IO_INCLUDE_DIR)
mark_as_advanced(GENERIC_IO_LIBRARIES)
