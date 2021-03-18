set(test_xmls
  IndeXRepresentation.xml
)
set(IndeXRepresentation_DISABLE_CS TRUE)
set(IndeXRepresentation_DISABLE_CRS TRUE)

# The embedded free IndeX license doesn't support parallel processing.
# Disable the test on MPI builds.
if (NOT PARAVIEW_USE_MPI)
  list(APPEND test_xmls
    IndeXRepresentationServer.xml)
  set(IndeXRepresentationServer_DISABLE_C TRUE)
  # This won't work until image delivery for the render server is
  # implemented.
  set(IndeXRepresentationServer_DISABLE_CRS TRUE)
endif ()

paraview_add_client_tests(
  BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
  TEST_DATA_TARGET ParaViewData
  TEST_SCRIPTS ${test_xmls}
)
paraview_add_client_server_tests(
  BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
  TEST_DATA_TARGET ParaViewData
  TEST_SCRIPTS ${test_xmls}
)

# Some setups may not have a new enough card to run IndeX. Skip the test on
# these machines.
set(index_tests
  pv.IndeXRepresentation)
if (NOT PARAVIEW_USE_MPI)
  list(APPEND index_tests
    pvcs.IndeXRepresentationServer)
endif ()
set_property(TEST ${index_tests} APPEND
  PROPERTY
    SKIP_REGULAR_EXPRESSION "The version of your CUDA driver is .*, but .* is the required minimum")
