# +---------------------------------------------------------------------------+
# |                                                                           |
# |                                  MPICH                                    |
# |                                                                           |
# +---------------------------------------------------------------------------+

include(FindMPI)
if (NOT MPI_FOUND)
  message(SEND_ERROR "MPI is required.")
endif(NOT MPI_FOUND)

add_definitions("-DMPICH_IGNORE_CXX_SEEK")

include_directories(${MPI_INCLUDE_PATH})

