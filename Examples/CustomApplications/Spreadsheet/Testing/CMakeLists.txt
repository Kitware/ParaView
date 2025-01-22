# More work required for MacOS. See #22889
if (APPLE)
  return()
endif()

if (TARGET ParaView::paraview)
  set (SmokeTest_USES_DIRECT_DATA ON)
  paraview_add_client_tests(
    CLIENT "$<TARGET_FILE:${app_name}>"
    TEST_SCRIPTS SmokeTest.xml
  )
endif()
