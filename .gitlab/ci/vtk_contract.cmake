# A contract file for testing ParaView within VTK's CI, with an installed VTK
# found through `CMAKE_PREFIX_PATH`. The configuration follows
# `configure_fedora44_shared_asserts_mpi_python_qt.cmake`, except for what
# VTK's CI image does not provide.

cmake_minimum_required(VERSION 3.20)
project(paraview-contract)

include(FetchContent)

if (NOT DEFINED PARAVIEW_CONTRACT_REPOSITORY)
  set(PARAVIEW_CONTRACT_REPOSITORY "https://gitlab.kitware.com/paraview/paraview.git")
endif ()
if (NOT DEFINED PARAVIEW_CONTRACT_BRANCH)
  set(PARAVIEW_CONTRACT_BRANCH "master")
endif ()

FetchContent_Declare(paraview
  GIT_REPOSITORY "${PARAVIEW_CONTRACT_REPOSITORY}"
  GIT_TAG "${PARAVIEW_CONTRACT_BRANCH}"
  GIT_SHALLOW ON
  # The `VTK` submodule is not needed.
  GIT_SUBMODULES
    ThirdParty/IceT/vtkicet
    ThirdParty/QtTesting/vtkqttesting
    Utilities/VisItBridge
  SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/paraview/src"
  # Only fetch the sources, without adding them to this project.
  SOURCE_SUBDIR do-not-add)
FetchContent_MakeAvailable(paraview)

# Configure ParaView along with this project, so that its configure output and
# warnings are reported as the configure step rather than the build step.
set(paraview_binary_dir "${CMAKE_CURRENT_BINARY_DIR}/paraview/build")
execute_process(
  COMMAND "${CMAKE_COMMAND}"
    -S "${paraview_SOURCE_DIR}"
    -B "${paraview_binary_dir}"
    -G "${CMAKE_GENERATOR}"
    "-DCMAKE_PREFIX_PATH:STRING=${CMAKE_PREFIX_PATH}"
    "-DCMAKE_C_COMPILER_LAUNCHER:STRING=${CMAKE_C_COMPILER_LAUNCHER}"
    "-DCMAKE_CXX_COMPILER_LAUNCHER:STRING=${CMAKE_CXX_COMPILER_LAUNCHER}"
    # Stock RelWithDebInfo flags, but without `-DNDEBUG` so that asserts are
    # active.
    "-DCMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING=-O2 -g"
    -DCMAKE_BUILD_TYPE=RelWithDebInfo
    # Use VTK's library directory, since ParaView's Python package is looked
    # for under VTK's site-packages directory.
    -DCMAKE_INSTALL_LIBDIR=lib
    # Otherwise `pv.TestDevelopmentInstall` relinks ParaView while other
    # tests load its libraries.
    -DCMAKE_SKIP_INSTALL_ALL_DEPENDENCY=ON
    # https://github.com/llnl/conduit/issues/1697
    -DCMAKE_POLICY_VERSION_MINIMUM=3.13
    -DCMAKE_POLICY_DEFAULT_CMP0167=NEW
    -DPARAVIEW_USE_EXTERNAL_VTK=ON
    -DPARAVIEW_USE_MPI=ON
    -DPARAVIEW_USE_PYTHON=ON
    -DPARAVIEW_USE_QT=ON
    -DPARAVIEW_USE_SERIALIZATION=ON
    -DPARAVIEW_BUILD_LEGACY_REMOVE=ON
    -DPARAVIEW_BUILD_TESTING=WANT
    -DPARAVIEW_ENABLE_EXAMPLES=ON
    -DPARAVIEW_ENABLE_EXTRA_BUILD_WARNINGS=ON
    -DPARAVIEW_LINKER_FATAL_WARNINGS=ON
    -DVTK_USE_LARGE_DATA=ON
    -DVTK_MODULE_USE_EXTERNAL_ParaView_protobuf=ON
    -DPARAVIEW_ENABLE_CATALYST=ON
    -DPARAVIEW_ENABLE_CATALYST_FIDES=ON
    -DPARAVIEW_ENABLE_ADIOS2=ON
    -DPARAVIEW_ENABLE_FFMPEG=ON
    -DPARAVIEW_ENABLE_FIDES=ON
    -DPARAVIEW_ENABLE_GDAL=ON
    -DPARAVIEW_ENABLE_OPENTURNS=ON
    -DPARAVIEW_ENABLE_PDAL=ON
    -DPARAVIEW_ENABLE_VISITBRIDGE=ON
    -DPARAVIEW_ENABLE_XDMF3=ON
    -DPARAVIEW_PLUGINS_DEFAULT=ON
    -DPARAVIEW_PLUGIN_ENABLE_NetCDFTimeAnnotationPlugin=ON
    -DPARAVIEW_PLUGIN_dsp_enable_audio_player=ON
    -DPARAVIEW_PLUGIN_ENABLE_ParFlow=ON
    -DPARAVIEW_PLUGIN_ENABLE_ONNXPlugin=ON
    -DPARAVIEW_PLUGIN_ENABLE_XRInterface=ON
    -DPARAVIEW_XRInterface_OpenVR_Support=ON
    -DPARAVIEW_XRInterface_OpenXR_Support=ON
  RESULT_VARIABLE paraview_configure_result)
if (paraview_configure_result)
  message(FATAL_ERROR "Failed to configure ParaView")
endif ()

add_custom_target(paraview ALL
  COMMAND "${CMAKE_COMMAND}" --build "${paraview_binary_dir}"
  # Show the output as it comes.
  USES_TERMINAL)

# Use ParaView's warning and test exceptions.
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake"
  "include(\"${paraview_binary_dir}/CTestCustom.cmake\")\n")

# Run ParaView's tests as tests of this project.
enable_testing()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/paraview-tests.cmake"
  "subdirs(\"${paraview_binary_dir}\")\n")
set_property(DIRECTORY APPEND PROPERTY
  TEST_INCLUDE_FILES "${CMAKE_CURRENT_BINARY_DIR}/paraview-tests.cmake")
