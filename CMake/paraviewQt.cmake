# PARAVIEW_QT_VERSION is used to choose between Qt5 and Qt6.
# If it is set to Auto(default), PARAVIEW finds and uses the
# version installed on the system. If both versions are
# found, Qt6 is preferred.

# Note that this is a duplication of vtkQt.cmake from VTK

set(paraview_supported_qt_versions "Auto" 5 6)

# The following `if` check can be removed once CMake 3.21 is required and
# the policy CMP0126 is set to NEW for ParaView and other superbuilds.
if (NOT DEFINED PARAVIEW_QT_VERSION)
  set(PARAVIEW_QT_VERSION "Auto" CACHE
    STRING "Expected Qt major version. Valid values are Auto, 5, 6.")
  set_property(CACHE PARAVIEW_QT_VERSION PROPERTY STRINGS "${paraview_supported_qt_versions}")
endif ()

if (NOT PARAVIEW_QT_VERSION IN_LIST paraview_supported_qt_versions)
  string(REPLACE ";" "\", \"" paraview_supported_qt_versions_string "${paraview_supported_qt_versions}")
  message(FATAL_ERROR
    "Supported Qt versions are \"${paraview_supported_qt_versions_string}\". "
    "But `PARAVIEW_QT_VERSION` is set to \"${PARAVIEW_QT_VERSION}\".")
endif ()

if (PARAVIEW_QT_VERSION STREQUAL "Auto")
  find_package(Qt6 QUIET COMPONENTS Core)
  set(_paraview_qt_version 6)
  if (NOT Qt6_FOUND)
    find_package(Qt5 QUIET COMPONENTS Core)
    if (NOT Qt5_FOUND)
      message(FATAL_ERROR
        "Could not find a valid Qt installation.")
    endif ()
    set(_paraview_qt_version 5)
  endif ()
else ()
  set(_paraview_qt_version "${PARAVIEW_QT_VERSION}")
endif ()

set(PARAVIEW_QT_MAJOR_VERSION "${_paraview_qt_version}")
