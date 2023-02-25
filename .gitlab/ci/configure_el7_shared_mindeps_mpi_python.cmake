# Only Qt 5.9 available.
set(PARAVIEW_USE_QT OFF CACHE BOOL "")

# VTK-m requires C++14
set(PARAVIEW_USE_VTKM OFF CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_el7.cmake")
