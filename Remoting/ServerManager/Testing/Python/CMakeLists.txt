# These tests depend on target ParaView::RemotingMisc, which may not
# exist in the edition being built. Exit
if (NOT TARGET ParaView::RemotingMisc)
  return()
endif ()

paraview_add_test_python(
  NO_VALID
  Settings.py
  TestHelperProxySerialization.py
  TestMultiplexerSourceProxy.py
  )
