
# Enable default-off plugin with Python dependency
set(PARAVIEW_PLUGIN_ENABLE_NetCDFTimeAnnotationPlugin ON CACHE BOOL "")
set(PARAVIEW_PLUGIN_dsp_enable_audio_player ON CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora_common.cmake")
