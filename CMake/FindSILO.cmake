# - try to find SILO library and include files
#
#  If SILO is not located somewhere obvious
#  then set SILO_DIR before running this file.
#
#  This file sets the following variables:
#
#  SILO_INCLUDE_DIR, where to find silo.h, etc.
#  SILO_LIBRARIES, the libraries to link against
#  SILO_FOUND, If false, do not try to use SILO.
#
# Also defined, but not for general use are:
#  SILO_LIBRARY, the full path to the silo library.
#  SILO_INCLUDE_PATH, for CMake backward compatibility

FIND_PATH( SILO_INCLUDE_DIR silo.h
  /usr/local/include
  /usr/include
)

FIND_LIBRARY( SILO_LIBRARY silo
  /usr/lib
  /usr/local/lib
)

SET( SILO_FOUND "NO" )
IF(SILO_INCLUDE_DIR)
  IF(SILO_LIBRARY)

    SET( SILO_LIBRARIES ${SILO_LIBRARY})
    SET( SILO_FOUND "YES" )

    #The following deprecated settings are for backwards compatibility with CMake1.4
    SET (SILO_INCLUDE_PATH ${SILO_INCLUDE_DIR})

  ENDIF(SILO_LIBRARY)
ENDIF(SILO_INCLUDE_DIR)

MARK_AS_ADVANCED(
  SILO_INCLUDE_DIR
  SILO_LIBRARY
)
