#    ____    _ __           ____               __    ____
#   / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
#  _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
# /___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)
#
# Copyright 2012 SciberQuest Inc.
#
set(SQTK_WITHOUT_MPI OFF CACHE BOOL "Build without mpi.")

if (SQTK_WITHOUT_MPI)
    message(STATUS "Building without MPI.")
    add_definitions(-DSQTK_WITHOUT_MPI)
else()
  include(FindMPI)
  if (NOT MPI_FOUND)
    message(STATUS "Building without MPI.")
    add_definitions(-DSQTK_WITHOUT_MPI)
  else()
    message(STATUS "Building with MPI.")
    add_definitions("-DMPICH_IGNORE_CXX_SEEK")
    include_directories(${MPI_INCLUDE_PATH})
  endif()
endif()
