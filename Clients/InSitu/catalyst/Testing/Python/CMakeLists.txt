# Catalyst V2 'fides_conduit' channel tests.
#
# These drive the Catalyst V2 conduit ABI directly through the catalyst SDK
# (the mini-app calls catalyst.execute()); the in-process Catalyst bridge used
# by paraview.demos.wavelet_miniapp cannot exercise a custom channel type. That
# means the test process needs the catalyst + catalyst_conduit + conduit Python
# modules on PYTHONPATH and needs to be able to locate the ParaView Catalyst
# implementation.

# Base conduit Python module (needed by catalyst_conduit).
get_target_property(_fides_conduit_py_dir VTK::conduit_python VTK_CONDUIT_PYTHON_MODULE_DIR)

# catalyst + catalyst_conduit Python modules, from the catalyst SDK.
find_package(catalyst 2.0 QUIET)
set(_fides_catalyst_py_dir)
if (CATALYST_PYTHONPATH)
  # CATALYST_PYTHONPATH may be a path-list; the first entry is the SDK's
  # site-packages holding the catalyst and catalyst_conduit packages.
  string(REPLACE ":" ";" _fides_catalyst_pp "${CATALYST_PYTHONPATH}")
  list(GET _fides_catalyst_pp 0 _fides_catalyst_py_dir)
endif ()

if (NOT _fides_conduit_py_dir OR NOT _fides_catalyst_py_dir OR NOT TARGET catalyst-paraview)
  message(STATUS
    "Skipping fides_conduit Catalyst tests: need conduit Python "
    "(${_fides_conduit_py_dir}), catalyst Python (${_fides_catalyst_py_dir}) "
    "and the catalyst-paraview implementation.")
  return ()
endif ()

set(_fides_catalyst_dir "${CMAKE_CURRENT_SOURCE_DIR}")

foreach (_case IN ITEMS unstructured cellgrid)
  set(_test_name "ParaView::Catalyst::FidesConduit::${_case}")
  add_test(
    NAME "${_test_name}"
    COMMAND "$<TARGET_FILE:ParaView::pvbatch>" --dr --sym
            "${_fides_catalyst_dir}/fides_catalyst_miniapp.py"
            --grid "${_case}"
            -s "${_fides_catalyst_dir}/fides_conduit_${_case}.py")
  set_tests_properties("${_test_name}"
    PROPERTIES
      PASS_REGULAR_EXPRESSION "All ok"
      ENVIRONMENT "CATALYST_IMPLEMENTATION_PATHS=$<TARGET_FILE_DIR:catalyst-paraview>")
  set_property(TEST "${_test_name}" APPEND PROPERTY
    ENVIRONMENT_MODIFICATION
      "PYTHONPATH=path_list_append:${_fides_catalyst_py_dir}"
      "PYTHONPATH=path_list_append:${_fides_conduit_py_dir}")

  # The CI image has multiple libcatalyst with the same soname (one per MPI
  # flavor under /usr and /usr/lib64/<mpi> using Catalyst's mangled Conduit,
  # plus one per MPI flavor under /opt/catalyst-ext using the external Conduit
  # installs. Make sure these tests load the one that matches the how ParaView
  # was built.
  if (UNIX AND NOT APPLE)
    set_property(TEST "${_test_name}" APPEND PROPERTY
      ENVIRONMENT_MODIFICATION
        "LD_LIBRARY_PATH=path_list_prepend:$<TARGET_FILE_DIR:catalyst::catalyst>")

    # Print a warning for older cmake versions
    if (CMAKE_VERSION VERSION_LESS "3.22")
      message(WARNING
        "CMake 3.22+ is required to reliably set up the environment for tests "
        "needing to load a specific Catalyst. ${_test_name} may fail to load "
        "the correct catalyst and fail as a result.")
    endif ()
  endif ()

endforeach ()
