# +---------------------------------------------------------------------------+
# |                                                                           |
# |                          Locate Eigen Install                             |
# |                                                                           |
# +---------------------------------------------------------------------------+

set(Eigen_DIR 
  ${CMAKE_CURRENT_SOURCE_DIR}/eigen-2.0.12/
  CACHE FILEPATH
  "Path to Eigen 2 install.")

if (NOT EXISTS ${Eigen_DIR})
  message(FATAL_ERROR
  "Set Eigen_DIR to the path to your Eigen install." )
endif ()

include_directories(${Eigen_DIR}/include/eigen2/)


# On Nautilus gcc intrinsics are found if
# we don't add an include for the intel intrinsics.
if (CMAKE_C_COMPILER_ID STREQUAL "Intel")

  message(STATUS "Intel compiler detected.")

  set(EXTRA_INTEL_INCLUDES
    /path/to/intel/include/intel64
    CACHE FILEPATH
    "Path to intel intrinsics.")

  if (NOT EXISTS ${EXTRA_INTEL_INCLUDES})
    message(FATAL_ERROR
      "Set EXTRA_INTEL_INCLUDES to the path to the intel compiler.")
  endif ()

  include_directories(${EXTRA_INTEL_INCLUDES})

endif ()

