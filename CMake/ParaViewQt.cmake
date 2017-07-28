# This module makes it easier to find and import appropriate Qt modules based
# on PARAVIEW_QT_VERSION.
# This was more useful when we supported Qt 4 and Qt 5 together. Now it's merely here
# to support Qt 5. We leave it around since it may make it easier when moving on to
# future major Qt releases.

#------------------------------------------------------------------------------
# Instead of doing `find_package(Qt5)`, use this macro to
# find an appropriate version based on PARAVIEW_QT_VERSION specified (which is currently
# forced to 5).
# You can pass all arguments typically passed to the find_package() call with
# following exceptions:
# - The first argument is a variable name that gets set to the list of targets
#   imported from the appropriate Qt package.
# - Do not pass a version number. That will not be processed correctly. The
#   version number is determined by this macro itself.
# - Do not use COMPONENTS or OPTIONAL_COMPONENTS instead, use
#   QT5_COMPONENTS, QT5_OPTIONAL_COMPONENTS to specify the components to use
#   for Qt5.
macro(pv_find_package_qt out_targets_var)
  if (DEFINED PARAVIEW_QT_VERSION AND PARAVIEW_QT_VERSION VERSION_LESS 5)
    message(WARNING "ParaView no longer supports ${PARAVIEW_QT_VERSION}. Forcing Qt version to 5.*.")
  endif()

  set(PARAVIEW_QT_VERSION 5)
  set(qt4_components)
  set(qt5_components)
  set(qt4_optional_components)
  set(qt5_optional_components)

  set(other_args)
  set(_doing)
  foreach(arg ${ARGN})
    if(arg MATCHES "^(QT(4|5)_COMPONENTS|QT(4|5)_OPTIONAL_COMPONENTS)$")
      set(_doing "${arg}")
    elseif(arg MATCHES "^(COMPONENTS|OPTIONAL_COMPONENTS)$")
      message(FATAL_ERROR "Qt4 and Qt5 components need to speciifed separately"
        "using QT4_COMPONENTS, QT4_OPTIONAL_COMPONENTS, QT5_COMPONENTS, or "
        "QT5_OPTIONAL_COMPONENTS keywords and not COMPONENTS or OPTIONAL_COMPONENTS.")
    elseif(_doing STREQUAL "QT4_COMPONENTS")
      list(APPEND qt4_components "${arg}")
    elseif(_doing STREQUAL "QT5_COMPONENTS")
      list(APPEND qt5_components "${arg}")
    elseif(_doing STREQUAL "QT4_OPTIONAL_COMPONENTS")
      list(APPEND qt4_optional_components "${arg}")
    elseif(_doing STREQUAL "QT5_OPTIONAL_COMPONENTS")
      list(APPEND qt5_optional_components "${arg}")
    else()
      set(_doing)
      list(APPEND other_args "${arg}")
    endif()
  endforeach()
  if(NOT qt5_components AND NOT qt5_optional_components)
    message(FATAL_ERROR "Components must be specified to find Qt correctly.")
  endif()
  if (qt4_components OR qt4_optional_components)
    message(AUTHOR_WARNING "Qt 4 components can be dropped since Qt 4 is no longer supported.")
  endif()

  set(_qt_targets)
  set(_qt_var_prefix Qt5)
  set(_qt_min_version "5.6")
  set(_qt_official_version "5.9")
  set(args)
  if(qt5_components)
    set(args ${args} COMPONENTS ${qt5_components})
  endif()
  if(qt5_optional_components)
    set(args ${args} OPTIONAL_COMPONENTS ${qt5_optional_components})
  endif()
  find_package(Qt5 ${_qt_min_version} ${args} ${other_args})
  set(_qt_version ${Qt5_VERSION})
  foreach(comp IN LISTS qt5_components qt5_optional_components)
    if(TARGET Qt5::${comp})
      list(APPEND _qt_targets "Qt5::${comp}")
    endif()
  endforeach()

  # Warn is Qt version is less than the official version.
  if(_qt_version VERSION_LESS _qt_official_version)
    get_property(_warned GLOBAL PROPERTY PARAVIEW_QT_VERSION_WARNING_GENERATED SET)
    if(NOT _warned)
      message(WARNING "Qt '${_qt_version}' found and will be used. "
        "Note, however, that the officially supported version is ${_qt_official_version}.")
      set_property(GLOBAL PROPERTY PARAVIEW_QT_VERSION_WARNING_GENERATED TRUE)
    endif()
  endif()

  # Add all imported components, we setup a variable that refers to the imported targets.
  set(${out_targets_var} ${_qt_targets})
endmacro()

macro(pv_qt_wrap_cpp)
  qt5_wrap_cpp(${ARGN})
endmacro()

macro(pv_qt_wrap_ui)
  qt5_wrap_ui(${ARGN})
endmacro()

macro(pv_qt_add_resources)
  qt5_add_resources(${ARGN})
endmacro()
