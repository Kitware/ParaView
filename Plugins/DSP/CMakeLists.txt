set(sources)
set(interfaces)
set(uifiles)

option(PARAVIEW_PLUGIN_dsp_enable_audio_player "Enable building the Audio Player Dock. Requires Qt Multimedia." OFF)
mark_as_advanced(PARAVIEW_PLUGIN_dsp_enable_audio_player)

if (PARAVIEW_USE_QT AND PARAVIEW_PLUGIN_dsp_enable_audio_player)
  find_package("Qt${PARAVIEW_QT_MAJOR_VERSION}" REQUIRED Widgets OPTIONAL_COMPONENTS Multimedia)

  if (NOT TARGET "Qt${PARAVIEW_QT_MAJOR_VERSION}::Multimedia")
    message(FATAL_ERROR
      "Component 'Qt5::Multimedia' required for the DSP plugin not found. Use "
      "PARAVIEW_PLUGIN_dsp_enable_audio_player to disable the audio player if necessary, or install 'Qt5::Multimedia'")
  endif ()

  list(APPEND sources
    pqAudioPlayer.cxx
    pqAudioPlayer.h)

  paraview_plugin_add_dock_window(
    CLASS_NAME pqAudioPlayer
    DOCK_AREA Right
    INTERFACES dock_interfaces
    SOURCES dock_sources)

  list(APPEND interfaces
    ${dock_interfaces})
  list(APPEND sources
    ${dock_sources})
  list(APPEND uifiles pqAudioPlayer.ui)
endif ()

paraview_add_plugin(DigitalSignalProcessing
  VERSION "1.0"
  UI_FILES ${uifiles}
  UI_INTERFACES ${interfaces}
  SOURCES ${sources}
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  MODULES
    DigitalSignalProcessing::DSPDataModel
    DigitalSignalProcessing::DSPFiltersPlugin
  MODULE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/DataModel/vtk.module"
    "${CMAKE_CURRENT_SOURCE_DIR}/Filters/vtk.module")

if (PARAVIEW_USE_QT AND PARAVIEW_PLUGIN_dsp_enable_audio_player)
  target_link_libraries(DigitalSignalProcessing
    PRIVATE
      "Qt${PARAVIEW_QT_MAJOR_VERSION}::Widgets"
      "Qt${PARAVIEW_QT_MAJOR_VERSION}::Multimedia")

  target_compile_definitions(DigitalSignalProcessing PRIVATE QT_NO_KEYWORDS)
endif()

if (BUILD_TESTING AND BUILD_SHARED_LIBS)
  add_subdirectory(Testing)
endif()
