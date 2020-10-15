set(CMAKE_PREFIX_PATH                   "/usr/lib64/openmpi" CACHE STRING "")
set(PARAVIEW_BUILD_SHARED_LIBS          ON                   CACHE BOOL "")
set(VTK_USE_X                           ON                   CACHE BOOL "")
set(CMAKE_C_COMPILER                    "icc"                CACHE STRING "")
set(CMAKE_CXX_COMPILER                  "icpc"               CACHE STRING "")
set(MPI_PREFLAGS                        "--mca orte_base_help_aggregate 0"  CACHE STRING "")
#set(PARAVIEW_MPI_PREFLAGS               "--mca orte_base_help_aggregate 0"  CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_common.cmake")
