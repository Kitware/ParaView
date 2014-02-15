#------------------------------------------------------------------------------
# Option and variables to define the Qt version to use
#------------------------------------------------------------------------------

SET( PARAVIEW_QT_VERSION "4" CACHE STRING "Expected Qt version" )
MARK_AS_ADVANCED( PARAVIEW_QT_VERSION )
SET_PROPERTY(CACHE PARAVIEW_QT_VERSION PROPERTY STRINGS 4 5)
IF( NOT ( PARAVIEW_QT_VERSION VERSION_EQUAL "4" OR
  PARAVIEW_QT_VERSION VERSION_EQUAL "5" ) )
  MESSAGE( FATAL_ERROR "Expected value for PARAVIEW_QT_VERSION is either '4' "
    "or '5'")
ENDIF()

SET( VTK_QT_VERSION ${PARAVIEW_QT_VERSION} CACHE STRING "" FORCE )

IF( PARAVIEW_QT_VERSION VERSION_GREATER "4" )
  SET( QT_MIN_VERSION "5.0.0" )
  SET( QT_OFFICIAL_VERSION "5.0.2" )
ELSE()
  SET( QT_MIN_VERSION "4.7.0" )
  SET( QT_OFFICIAL_VERSION "4.8" )
ENDIF()
