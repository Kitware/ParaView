# Adding test should be mainly add XML under one of the following list:
# * TESTS_WITHOUT_BASELINES
# * TESTS_WITH_INLINE_COMPARES
# * TESTS_WITH_BASELINES
# Please do that in an included CMake script, to keep this file small.

set(TESTS_WITHOUT_BASELINES)
set(TESTS_WITH_INLINE_COMPARES)
set(TESTS_WITH_BASELINES)

include(test.common.cmake)

if (PARAVIEW_USE_MPI)
  include(test.mpi.cmake)
endif()

if(PARAVIEW_USE_PYTHON)
  include(test.python.cmake)
endif()

if (PARAVIEW_USE_MPI AND PARAVIEW_USE_PYTHON)
  include(test.mpi-python.cmake)
endif ()

option(PARAVIEW_SSH_SERVERS_TESTING
    "Add SSH Servers testing"
    OFF)
mark_as_advanced(PARAVIEW_SSH_SERVERS_TESTING)

if(PARAVIEW_SSH_SERVERS_TESTING)
  include(test.ssh.cmake)
endif(PARAVIEW_SSH_SERVERS_TESTING)

# Finally create the tests from the XML lists
list(APPEND TESTS_WITHOUT_BASELINES
  ${TESTS_WITH_INLINE_COMPARES})

paraview_add_client_tests(
  TEST_SCRIPTS ${TESTS_WITHOUT_BASELINES}
)

paraview_add_client_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS ${TESTS_WITH_BASELINES}
)

paraview_add_client_server_tests(
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS ${TESTS_WITH_BASELINES}
)

paraview_add_client_server_tests(
  TEST_SCRIPTS ${TESTS_WITH_INLINE_COMPARES}
)

if (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
  paraview_add_client_server_render_tests(
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${TESTS_WITH_BASELINES}
  )

  paraview_add_client_server_render_tests(
    TEST_SCRIPTS ${TESTS_WITH_INLINE_COMPARES}
  )
endif ()
