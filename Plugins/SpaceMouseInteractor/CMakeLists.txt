# so we can use our local Find3DxWareSDK.cmake
list(INSERT CMAKE_MODULE_PATH 0
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(interfaces)
set(sources)
if (PARAVIEW_USE_QT)
  list(APPEND sources
    pqSpaceMouseAutoStart.cxx
    pqSpaceMouseAutoStart.h)
  paraview_plugin_add_auto_start(
    CLASS_NAME pqSpaceMouseAutoStart
    INTERFACES autostart_interface
    SOURCES autostart_sources)
  list(APPEND interfaces
    ${autostart_interface})
  list(APPEND sources
    ${autostart_sources})
  if (WIN32)
    list(APPEND sources
      pqSpaceMouseImplWin.cxx
      pqSpaceMouseImplWin.h)
    find_package(3DxWareSDK)
  else()
    # this is a do-nothing implementation for now
    list(APPEND sources
      pqSpaceMouseImplLinux.cxx
      pqSpaceMouseImplLinux.h)
  endif()
endif ()


set(CMAKE_CXX_STANDARD 11)
paraview_add_plugin(SpaceMouseInteractor
  VERSION       "1.0"
  UI_INTERFACES ${interfaces}
  SOURCES       ${sources})

if (PARAVIEW_USE_QT)
  target_link_libraries(SpaceMouseInteractor
    PRIVATE
      ParaView::RemotingApplication
      ParaView::RemotingCore
      ParaView::RemotingServerManager
      ParaView::RemotingViews
      ParaView::pqApplicationComponents)
  if (WIN32)
    target_link_libraries(SpaceMouseInteractor
      PRIVATE
        3Dconnexion::3DxWareSDK)
  endif()
    target_compile_definitions(SpaceMouseInteractor PRIVATE QT_NO_KEYWORDS)
endif ()
