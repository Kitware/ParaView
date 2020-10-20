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

# Set RPATH with MPI
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "mpi")
  set(CMAKE_INSTALL_RPATH "/usr/lib64/openmpi")
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif ()

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

# Shared/static
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "shared")
  set(PARAVIEW_BUILD_SHARED_LIBS ON CACHE BOOL "")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "static")
  set(PARAVIEW_BUILD_SHARED_LIBS OFF CACHE BOOL "")
endif ()
