
# Enable default-off plugin with Python dependency
set(PARAVIEW_PLUGIN_ENABLE_NetCDFTimeAnnotationPlugin ON CACHE BOOL "")
set(PARAVIEW_PLUGIN_dsp_enable_audio_player ON CACHE BOOL "")

# Turn on constant implicit array dispatch instantiation
# This is required for testing the DSP Plugin
set(VTK_DISPATCH_CONSTANT_ARRAYS ON CACHE BOOL "")

# Something ends up loading the system protobuf and conflicting with our
# vendored copy. Just use the system copy.
set(VTK_MODULE_USE_EXTERNAL_ParaView_protobuf ON CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora_common.cmake")
