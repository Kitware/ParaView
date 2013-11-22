#    ____    _ __           ____               __    ____
#   / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
#  _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
# /___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)
#
# Copyright 2012 SciberQuest Inc.
#
project(SciberQuestToolKit)

#--------------------------------------------------
# Use ParaView configuration for MPI
#--------------------------------------------------
if(PARAVIEW_USE_MPI)
  add_definitions("-DMPICH_IGNORE_CXX_SEEK")
else()
  add_definitions(-DSQTK_WITHOUT_MPI)
endif()

set(SCIBERQUEST_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/SciberQuest
  ${CMAKE_CURRENT_BINARY_DIR}/SciberQuest
  )

# process all vtk-modules defined in this plugin. This generates the necessary
# wrappings for all these modules as well.
pv_process_modules()

include_directories(${SCIBERQUEST_INCLUDE_DIRS})

add_subdirectory(ParaViewPlugin)
