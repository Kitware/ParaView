# These tests depend on target ParaView::RemotingMisc, which may not
# exist in the edition being built. Exit
if (NOT TARGET ParaView::RemotingMisc)
  return()
endif ()

ExternalData_Expand_Arguments(ParaViewData _
  # data files
  "DATA{${paraview_test_data_directory_input}/Data/globbing/glob0.png}"
  "DATA{${paraview_test_data_directory_input}/Data/globbing/glob1.png}"
  "DATA{${paraview_test_data_directory_input}/Data/globbing/glob2.png}"
  "DATA{${paraview_test_data_directory_input}/Data/globbing/glob3.png}"
  )

paraview_add_test_python(
  NO_VALID
  Settings.py
  TestHelperProxySerialization.py
  TestMultiplexerSourceProxy.py
  TestGlobbing.py
  TestGetRenderViewAfterConnectToCatalyst.py
  )
