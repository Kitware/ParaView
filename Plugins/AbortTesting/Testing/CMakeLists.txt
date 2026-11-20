if (BUILD_SHARED_LIBS)

  ExternalData_Expand_Arguments("ParaViewData" _
    "DATA{Data/unbalanced.vtkhdf}"
  )

  paraview_add_client_tests(
    TEST_SCRIPTS  AbortPipeline.xml)

  if (PARAVIEW_USE_MPI)
    paraview_add_client_server_tests(
      TEST_SCRIPTS  AbortPipelineCS.xml
      NUMSERVERS 3)

    paraview_add_client_server_render_tests(
      TEST_SCRIPTS  AbortPipelineCS.xml
      NUMSERVERS 3)

    paraview_add_client_server_tests(
      TEST_SCRIPTS AbortPipelineCSUnbalanced.xml
      NUMSERVERS 3)

    # Same as AbortPipelineCSUnbalanced with different NUMSERVERS
    paraview_add_client_server_tests(
      TEST_SCRIPTS AbortPipelineCSUndersubscribe.xml
      NUMSERVERS 4)

    # Same as AbortPipelineCSUnbalanced with different NUMSERVERS
    paraview_add_client_server_tests(
      TEST_SCRIPTS AbortPipelineCSOversubscribe.xml
      NUMSERVERS 2)

  endif ()
endif ()
