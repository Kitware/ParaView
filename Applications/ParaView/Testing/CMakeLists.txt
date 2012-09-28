option(PARAVIEW_CLIENT_RENDER_SERVER_TESTS
  "Turn on/off client tests that involve separate data and render servers."
  ON)
mark_as_advanced(PARAVIEW_CLIENT_RENDER_SERVER_TESTS)

include(ParaViewTestingMacros)
add_subdirectory(XML)
if (QT_TESTING_WITH_PYTHON)
  add_subdirectory(Python)
endif (QT_TESTING_WITH_PYTHON)
