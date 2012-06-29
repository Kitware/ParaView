#    ____    _ __           ____               __    ____
#   / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
#  _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
# /___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)
#
# Copyright 2012 SciberQuest Inc.
#
set(ParaView_DIR
  /path/to/paraview/build
  CACHE FILEPATH
  "Path to ParaView build.")

if (NOT EXISTS ${ParaView_DIR})
  MESSAGE(FATAL_ERROR
  "Set ParaView_DIR to the path to a ParaView build." )
endif (NOT EXISTS ${ParaView_DIR})

find_package(ParaView REQUIRED)
include(${PARAVIEW_USE_FILE})

message(STATUS "ParaView ${PARAVIEW_VERSION_FULL} found.")
