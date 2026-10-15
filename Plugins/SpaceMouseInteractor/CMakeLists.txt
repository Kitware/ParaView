# Use the plugin's local find modules for 3DxWareSDK and libspnav.
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
  cmake_policy(PUSH)
  if (POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
  endif ()
  find_package(Spnav REQUIRED)
  cmake_policy(POP)
  list(APPEND sources
    pqSpaceMouseImplLinux.cxx
    pqSpaceMouseImplLinux.h)
endif ()

# Support std::make_unique.
set(CMAKE_CXX_STANDARD 14)

if (WIN32 OR APPLE)
paraview_add_plugin(SpaceMouseInteractor
  REQUIRED_ON_CLIENT
  VERSION       "1.0"
  EULA "LicenseAgreementSDK.txt"
  DOCUMENTATION_DIR "${CMAKE_CURRENT_SOURCE_DIR}/doc"
  UI_INTERFACES ${interfaces}
  SOURCES       ${sources})
else ()
paraview_add_plugin(SpaceMouseInteractor
  REQUIRED_ON_CLIENT
  VERSION       "1.0"
  DOCUMENTATION_DIR "${CMAKE_CURRENT_SOURCE_DIR}/doc"
  UI_INTERFACES ${interfaces}
  SOURCES       ${sources})
endif()

# Support __declspec(property) on macOS.
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
else ()
  target_link_libraries(SpaceMouseInteractor
    PRIVATE
      Spnav::Spnav)
endif ()
target_compile_definitions(SpaceMouseInteractor PRIVATE QT_NO_KEYWORDS)
