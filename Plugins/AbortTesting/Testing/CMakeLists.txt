# Add AbortPipeline tests
# Force NUM_SERVER to 1 for client/server tests because the feature is not implemented for multiple server rank
paraview_add_client_tests(
  TEST_SCRIPTS AbortPipeline.xml
)
paraview_add_client_server_tests(
  TEST_SCRIPTS AbortPipeline.xml
  NUMSERVERS 1
)
paraview_add_client_server_render_tests(
  TEST_SCRIPTS AbortPipeline.xml
  NUMSERVERS 1
)
