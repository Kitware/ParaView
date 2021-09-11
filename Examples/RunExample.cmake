cmake_minimum_required(VERSION 3.12)

if (WIN32)
  # Append the ParaView DLL directory to PATH for the tests.
  set(ENV{PATH}
    "$ENV{PATH};${paraview_binary_dir}")
endif ()

set(cmake_arguments)
if (platform)
  list(APPEND cmake_arguments
    -A "${platform}")
endif ()
if (toolset)
  list(APPEND cmake_arguments
    -T "${toolset}")
endif ()

execute_process(
  COMMAND
    "${ctest}"
    --build-generator
      "${generator}"
    --build-and-test
      "${source}/${example_dir}"
      "${binary}/${example_dir}"
    --build-options
      ${cmake_arguments}
      "-DBUILD_TESTING:BOOL=ON"
      "-DCMAKE_BUILD_TYPE:STRING=${build_type}"
      "-DBUILD_SHARED_LIBS:BOOL=${shared}"
      "-DParaView_DIR:PATH=${paraview_dir}"
      "-DParaView_CATALYST_DIR:PATH=${paraview_catalyst_dir}"
    --test-command
      "${ctest}"
        -C ${config}
        --output-on-failure
  RESULT_VARIABLE res)

if (res)
  message(FATAL_ERROR "Test command failed")
endif ()
