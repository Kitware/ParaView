#########################################################################
# Everything under Application uses VTK/ParaView ServerManager.
include_directories(${VTK_INCLUDE_DIRS})
link_directories(${VTK_LIBRARY_DIRS})

include(ParaViewBranding)
include(ParaViewBrandingCPack)

if (PARAVIEW_BUILD_QT_GUI)
  add_subdirectory(ParaView)
endif()
