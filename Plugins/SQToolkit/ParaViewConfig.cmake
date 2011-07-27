# +---------------------------------------------------------------------------+
# |                                                                           |
# |                          Locate ParaView build                            |
# |                                                                           |
# +---------------------------------------------------------------------------+
#ParaView3
set(ParaView_DIR 
  /path/to/paraview/build
  CACHE FILEPATH
  "Path to ParaView build.")

if (NOT EXISTS ${ParaView_DIR})
  MESSAGE( FATAL_ERROR 
  "Set ParaView_DIR to the path to a ParaView build." )
endif (NOT EXISTS ${ParaView_DIR})

find_package(ParaView REQUIRED)
include(${PARAVIEW_USE_FILE})

