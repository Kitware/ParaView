include("${CMAKE_CURRENT_LIST_DIR}/gitlab_ci.cmake")

# Read the files from the build directory.
ctest_read_custom_files("${CTEST_BINARY_DIRECTORY}")

# Pick up from where the configure left off.
ctest_start(APPEND)

include(ProcessorCount)
ProcessorCount(nproc)

# Default to a reasonable test timeout.
set(CTEST_TEST_TIMEOUT 100)

set(test_exclusions
)

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos")
  list(APPEND test_exclusions
    # Segfaults in an event handler
    "\\.ColorOpacityTableEditing$"

    # Possibly https://gitlab.kitware.com/paraview/paraview/-/issues/19091
    "\\.SeparateOpacityArray$"

    # Known-bad
    "\\.SliceWithPlaneMultiBlock$"
    "\\.PreviewFontScaling$"

    # Unstructured grid volume rendering (paraview/paraview#19130)
    "\\.MultiBlockVolumeRendering$"
    "\\.UnstructuredVolumeRenderingVectorComponent$")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "mpi")
  set(ENV{OMPI_ALLOW_RUN_AS_ROOT_CONFIRM} "1")
  set(ENV{OMPI_ALLOW_RUN_AS_ROOT} "1")

  if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "linux")
    list(APPEND test_exclusions 

      # Known-bad https://gitlab.kitware.com/paraview/paraview/-/issues/20108
      "^ParaViewExample-Catalyst$")
  endif ()
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "icc")
  set(ENV{CC}  "icc")
  set(ENV{CXX} "icpc")
endif ()

string(REPLACE ";" "|" test_exclusions "${test_exclusions}")
if (test_exclusions)
  set(test_exclusions "(${test_exclusions})")
endif ()

ctest_test(APPEND
  PARALLEL_LEVEL "${nproc}"
  RETURN_VALUE test_result
  EXCLUDE "${test_exclusions}"
  REPEAT UNTIL_PASS:3)
ctest_submit(PARTS Test)

if (test_result)
  message(FATAL_ERROR
    "Failed to test")
endif ()
