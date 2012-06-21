#    ____    _ __           ____               __    ____
#   / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
#  _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
# /___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)
#
# Copyright 2012 SciberQuest Inc.
#
set(Eigen_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/eigen-3.0.3/eigen-eigen-3.0.3
  CACHE FILEPATH
  "Path to Eigen install.")

if (NOT EXISTS ${Eigen_DIR})
  message(FATAL_ERROR
  "Set Eigen_DIR to the path to your Eigen install." )
endif ()

include_directories(${Eigen_DIR})

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
