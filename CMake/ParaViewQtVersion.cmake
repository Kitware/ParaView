#------------------------------------------------------------------------------
# Option and variables to define the Qt version to use
#------------------------------------------------------------------------------

set(PARAVIEW_QT_VERSION "4" CACHE STRING "Expected Qt version")
mark_as_advanced(VTK_QT_VERSION)
set_property(CACHE PARAVIEW_QT_VERSION PROPERTY STRINGS 4 5)
if(NOT (PARAVIEW_QT_VERSION VERSION_EQUAL "4" OR
  PARAVIEW_QT_VERSION VERSION_EQUAL "5"))
  message(FATAL_ERROR "Expected value for PARAVIEW_QT_VERSION is either '4' "
    "or '5'")
endif()

set(VTK_QT_VERSION ${PARAVIEW_QT_VERSION} CACHE STRING "" FORCE)

if(PARAVIEW_QT_VERSION VERSION_GREATER "4")
  set(QT_MIN_VERSION "5.0.0")
  set(QT_OFFICIAL_VERSION "5.0.2")
else()
  set(QT_MIN_VERSION "4.7.0")
  set(QT_OFFICIAL_VERSION "4.8")
endif()
