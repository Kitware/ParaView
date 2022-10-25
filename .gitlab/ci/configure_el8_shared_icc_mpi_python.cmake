# Disable floating point optimizations which break `isinf` and `isnan`.
set(CMAKE_C_FLAGS "-fp-model=precise" CACHE STRING "")
set(CMAKE_CXX_FLAGS "-fp-model=precise" CACHE STRING "")
set(CMAKE_Fortran_FLAGS "-fp-model=precise" CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_el8.cmake")
