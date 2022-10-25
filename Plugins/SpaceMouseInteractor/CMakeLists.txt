# so we can use our local Find3DxWareSDK.cmake
list(INSERT CMAKE_MODULE_PATH 0
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(interfaces)
set(sources)

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

if (WIN32 OR APPLE)
  list(APPEND sources
    pqSpaceMouseImpl.cxx
    pqSpaceMouseImpl.h)
  find_package(3DxWareSDK REQUIRED)
else ()
  # this is a do-nothing implementation until 3DConnexion releases support.
  list(APPEND sources
    pqSpaceMouseImplLinux.cxx
    pqSpaceMouseImplLinux.h)
endif ()

# support std::make_unique
set(CMAKE_CXX_STANDARD 14)

paraview_add_plugin(SpaceMouseInteractor
  REQUIRED_ON_CLIENT
  VERSION       "1.0"
  UI_INTERFACES ${interfaces}
  SOURCES       ${sources})

# support __declspec(property) on Mac
target_compile_options(SpaceMouseInteractor
  PRIVATE
    "$<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang>:-fdeclspec>")

target_link_libraries(SpaceMouseInteractor
  PRIVATE
    ParaView::RemotingApplication
    ParaView::RemotingCore
    ParaView::RemotingServerManager
    ParaView::RemotingViews
    ParaView::pqApplicationComponents)
if (WIN32 OR APPLE)
  target_link_libraries(SpaceMouseInteractor
    PRIVATE
      3Dconnexion::3DxWareSDK)
endif ()
target_compile_definitions(SpaceMouseInteractor PRIVATE QT_NO_KEYWORDS)
