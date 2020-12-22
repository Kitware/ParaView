# Options that can be overridden based on the
# configuration name.
function (configuration_flag variable configuration)
  if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "${configuration}")
    set("${variable}" ON CACHE BOOL "")
  else ()
    set("${variable}" OFF CACHE BOOL "")
  endif ()
endfunction ()

# kits
configuration_flag(PARAVIEW_BUILD_WITH_KITS "kits")

# mpi
configuration_flag(PARAVIEW_USE_MPI "mpi")

# offscreen
configuration_flag(VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN "offscreen")
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "offscreen")
  set(VTK_USE_COCOA OFF CACHE BOOL "")
  set(VTK_USE_X OFF CACHE BOOL "")
endif ()

# osmesa
configuration_flag(VTK_OPENGL_HAS_OSMESA "osmesa")

# python
configuration_flag(PARAVIEW_USE_PYTHON "python")

# qt
configuration_flag(PARAVIEW_USE_QT "qt")

# edition
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "catalyst_rendering")
  set(PARAVIEW_BUILD_ALL_MODULES OFF CACHE BOOL "")
  set(PARAVIEW_BUILD_EDITION "CATALYST_RENDERING" CACHE STRING "")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "catalyst")
  set(PARAVIEW_BUILD_ALL_MODULES OFF CACHE BOOL "")
  set(PARAVIEW_BUILD_EDITION "CATALYST" CACHE STRING "")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "core")
  set(PARAVIEW_BUILD_ALL_MODULES OFF CACHE BOOL "")
  set(PARAVIEW_BUILD_EDITION "CORE" CACHE STRING "")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "rendering")
  set(PARAVIEW_BUILD_ALL_MODULES OFF CACHE BOOL "")
  set(PARAVIEW_BUILD_EDITION "RENDERING" CACHE STRING "")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "canonical")
  set(PARAVIEW_BUILD_ALL_MODULES OFF CACHE BOOL "")
  set(PARAVIEW_BUILD_EDITION "CANONICAL" CACHE STRING "")
elseif()
  set(PARAVIEW_BUILD_ALL_MODULES ON CACHE BOOL "")
  set(PARAVIEW_BUILD_EDITION "CANONICAL" CACHE STRING "")
endif()

# Shared/static
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "shared")
  set(PARAVIEW_BUILD_SHARED_LIBS ON CACHE BOOL "")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "static")
  set(PARAVIEW_BUILD_SHARED_LIBS OFF CACHE BOOL "")
endif ()
