include(FindPythonModules)
find_python_module(numpy numpy_found)
if (NOT PARAVIEW_USE_PYTHON OR NOT numpy_found)
  message(STATUS "Cannot test plugin DigitalSignalProcessing, missing Python + numpy support")
  return()
endif()

set (tests_with_baselines
  "${CMAKE_CURRENT_SOURCE_DIR}/MeanPowerSpectralDensity.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/MergeReduceTableBlocks.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/ProjectSpectrumMagnitude.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/SoundQuantities.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/Spectrogram.xml")
set (SoundQuantities_FORCE_SERIAL TRUE)

paraview_add_client_tests(
  LOAD_PLUGIN "DigitalSignalProcessing"
  BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
  TEST_DATA_TARGET ParaViewData
  TEST_SCRIPTS "${tests_with_baselines}")

# server tests are disabled for now until the MPI version are
# implemented for the following filters :
# - SoundQuantities
# - MergeReduceTableBlocks
# - ProjectSpectrumMagnitude
# - MeanPowerSpectralDensity

# add test dependant to Qt
if (PARAVIEW_USE_QT AND PARAVIEW_PLUGIN_dsp_enable_audio_player AND numpy_found)
  paraview_add_client_tests(
    LOAD_PLUGIN "DigitalSignalProcessing"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/AudioPlayer.xml")
  paraview_add_client_server_tests(
    LOAD_PLUGIN "DigitalSignalProcessing"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/AudioPlayer.xml")
  paraview_add_client_server_render_tests(
    LOAD_PLUGIN "DigitalSignalProcessing"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/AudioPlayer.xml")
endif()
