# This module makes it easier to find and import appropriate Qt modules based
# on PARAVIEW_QT_VERSION.

#------------------------------------------------------------------------------
# Instead of doing find_package(Qt4) or find_package(Qt5), use this macro to
# find an appropriate version based on PARAVIEW_QT_VERSION specified.
# You can pass all arguments typically passed to the find_package() call with
# following exceptions:
# - The first argument is a variable name that gets set to the list of targets
#   imported from the appropriate Qt package.
# - Do not pass a version number. That will not be processed correctly. The
#   version number is determined by this macro itself.
# - Do not use COMPONENTS or OPTIONAL_COMPONENTS instead, use QT4_COMPONENTS,
#   QT4_OPTIONAL_COMPONENTS and QT5_COMPONENTS, QT5_COMPONENTS to separately
#   specify the components to use for Qt4 and Qt5.
macro(pv_find_package_qt out_targets_var)
  if(NOT DEFINED PARAVIEW_QT_VERSION)
    set(PARAVIEW_QT_VERSION "5")
  endif()

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
  if(NOT (qt4_components OR qt5_components OR qt4_optional_components OR qt5_optional_components))
    message(FATAL_ERROR "Components must be specified to find Qt correctly.")
  endif()

  set(_qt_targets)
  if(PARAVIEW_QT_VERSION VERSION_GREATER "4")
    set(_qt_var_prefix Qt5)
    set(_qt_min_version "5.6")
    set(_qt_official_version "5.6")

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
  else()
    set(_qt_var_prefix Qt4)
    set(_qt_min_version "4.7")
    set(_qt_official_version "4.8")

    set(args)
    if(qt4_components)
      set(args COMPONENTS ${qt4_components})
    endif()
    if(qt4_optional_components)
      set(arg OPTIONAL_COMPONENTS ${qt4_optional_components})
    endif()

    #---------------------------------------------------------
    set(QT_USE_IMPORTED_TARGETS TRUE)
    find_package(Qt4 ${_qt_min_version} ${args} ${other_args})
    set(_qt_version ${QTVERSION})
    foreach(comp IN LISTS qt4_components qt4_optional_components)
      if(TARGET Qt4::${comp})
        list(APPEND _qt_targets "Qt4::${comp}")
      endif()
    endforeach()
  endif()

  # Warn is Qt version is less than the official version.
  if(_qt_version VERSION_LESS _qt_official_version)
    message(WARNING "You are using Qt ${_qt_version}. "
      "Officially supported version is ${_qt_official_version}")
  endif()

  # Add all imported components, we setup a variable that refers to the imported targets.
  set(${out_targets_var} ${_qt_targets})
endmacro()

macro(pv_qt_wrap_cpp)
  if(PARAVIEW_QT_VERSION VERSION_GREATER "4")
    qt5_wrap_cpp(${ARGN})
  else()
    qt4_wrap_cpp(${ARGN})
  endif()
endmacro()

macro(pv_qt_wrap_ui)
  if(PARAVIEW_QT_VERSION VERSION_GREATER "4")
    qt5_wrap_ui(${ARGN})
  else()
    qt4_wrap_ui(${ARGN})
  endif()
endmacro()

macro(pv_qt_add_resources)
  if(PARAVIEW_QT_VERSION VERSION_GREATER "4")
    qt5_add_resources(${ARGN})
  else()
    qt4_add_resources(${ARGN})
  endif()
endmacro()
