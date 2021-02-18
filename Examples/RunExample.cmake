cmake_minimum_required(VERSION 3.12)

if (WIN32)
  # Append the ParaView DLL directory to PATH for the tests.
  list(APPEND ENV{PATH}
    "${paraview_binary_dir}")
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
      "-DBUILD_TESTING:BOOL=ON"
      "-DCMAKE_BUILD_TYPE:STRING=${build_type}"
      "-DBUILD_SHARED_LIBS:BOOL=${shared}"
      "-DParaView_DIR:PATH=${paraview_dir}"
    --test-command
      "${ctest}"
        -C ${config}
  RESULT_VARIABLE res)

if (res)
  message(FATAL_ERROR "Test command failed")
endif ()
