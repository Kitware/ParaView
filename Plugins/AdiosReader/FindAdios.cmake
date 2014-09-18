# =================================================
# Find Adios library and set the following vars
# - ADIOS_INCLUDE_PATH
# - ADIOS_LIBRARY
# - ADIOS_READ_LIBRARY
# - ADIOS_READ_NO_MPI_LIBRARY
# =================================================

FIND_PATH(ADIOS_INCLUDE_PATH adios.h
    /usr/include
    /usr/local/include
    /opt/adios/include
)

IF (ADIOS_INCLUDE_PATH)
  FIND_LIBRARY(ADIOS_LIBRARY
    NAMES
      adios
    PATHS
      ${ADIOS_INCLUDE_PATH}/../lib
      /usr/lib
      /usr/local/lib
      /opt/adios/lib
  )

  FIND_LIBRARY(ADIOS_READ_LIBRARY
    NAMES
      adiosread
    PATHS
      ${ADIOS_INCLUDE_PATH}/../lib
      /usr/lib
      /usr/local/lib
      /opt/adios/lib
  )

  FIND_LIBRARY(ADIOS_READ_NO_MPI_LIBRARY
    NAMES
      adiosread_nompi
    PATHS
      ${ADIOS_INCLUDE_PATH}/../lib
      /usr/lib
      /usr/local/lib
      /opt/adios/lib
  )

ENDIF ()
