# +---------------------------------------------------------------------------+
# |                                                                           |
# |                          Locate ParaView build                            |
# |                                                                           |
# +---------------------------------------------------------------------------+
#ParaView3
if (UNIX OR CYGWIN)
  # +------------------+
  # | Unix/Linux/Cygwin|
  # +------------------+
  set(ParaView_DIR 
    /home/burlen/ext/kitware_cvs/PV3-VisIt
    CACHE FILEPATH
    "Path to ParaView build.")
else (UNIX OR CYGWIN)
  # +------------------+
  # |     Windows      |
  # +------------------+
  set(ParaView_DIR 
    C:/PV3
    CACHE FILEPATH
    "Path to ParaView build.")
endif (UNIX OR CYGWIN)

if (NOT EXISTS ${ParaView_DIR})
  MESSAGE( FATAL_ERROR 
  "Set ParaView_DIR to the path to your local ParaView3 out of source build." )
endif (NOT EXISTS ${ParaView_DIR})

find_package(ParaView REQUIRED)
include(${PARAVIEW_USE_FILE})
