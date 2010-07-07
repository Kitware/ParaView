PROJECT(AdaptiveParaView)

INCLUDE(${ParaView_CMAKE_DIR}/ParaViewBranding.cmake)
INCLUDE(${ParaView_CMAKE_DIR}/ParaViewPlugins.cmake)

add_subdirectory(Plugin)

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  pqCustomViewModules.h
  pqCustomDisplayPolicy.h
  adaptiveMainWindow.h
)

QT4_WRAP_UI(UI_BUILT_SOURCES
  adaptiveMainWindow.ui
)

SET(Application_SOURCE_FILES
  adaptiveMainWindow.cxx
  adaptiveMainWindow.h
  pqCustomViewModules.cxx
  pqCustomDisplayPolicy.cxx
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES}
)

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

build_paraview_client(adaptive_paraview
  TITLE "ParaView (Adaptive)"
  ORGANIZATION  "Kitware Inc."
  VERSION_MAJOR 1 
  VERSION_MINOR 1
  VERSION_PATCH 1
#  SPLASH_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/RSplash.png"
  PVMAIN_WINDOW adaptiveMainWindow
  PVMAIN_WINDOW_INCLUDE adaptiveMainWindow.h
  REQUIRED_PLUGINS AdaptivePlugin
  GUI_CONFIGURATION_XMLS
    ${CMAKE_CURRENT_SOURCE_DIR}/CustomResources/CustomWriters.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/CustomResources/CustomReaders.xml 
    ${CMAKE_CURRENT_SOURCE_DIR}/CustomResources/CustomFilters.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/CustomResources/CustomSources.xml
  SOURCES ${Application_SOURCE_FILES}
  INSTALL_BIN_DIR "${PV_INSTALL_BIN_DIR}"
  INSTALL_LIB_DIR "${PV_INSTALL_LIB_DIR}"
)

ADD_EXECUTABLE(ppRawStridedReader2 ppRawStridedReader2.cxx)
TARGET_LINK_LIBRARIES(ppRawStridedReader2 vtksys)
# ADD_EXECUTABLE(ppACosmoReader ppACosmoReader.cxx)
