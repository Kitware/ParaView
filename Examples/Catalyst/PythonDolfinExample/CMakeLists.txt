set(DOLFIN_EXAMPLE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(DOLFIN_EXAMPLE_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(DOLFIN_EXAMPLE_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/run-original.sh.in
  ${CMAKE_CURRENT_BINARY_DIR}/run-original.sh
  @ONLY)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/run-catalyst-step1.sh.in
  ${CMAKE_CURRENT_BINARY_DIR}/run-catalyst-step1.sh
  @ONLY)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/run-catalyst-step2.sh.in
  ${CMAKE_CURRENT_BINARY_DIR}/run-catalyst-step2.sh
  @ONLY)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/run-catalyst-step3.sh.in
  ${CMAKE_CURRENT_BINARY_DIR}/run-catalyst-step3.sh
  @ONLY)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/run-catalyst-step4.sh.in
  ${CMAKE_CURRENT_BINARY_DIR}/run-catalyst-step4.sh
  @ONLY)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/run-catalyst-step6.sh.in
  ${CMAKE_CURRENT_BINARY_DIR}/run-catalyst-step6.sh
  @ONLY)

# TODO: `sys.path` is not set up properly in the following configure.
# `pvpython` should probably be asked for the Python module location. Or it
# could just be added to `paraview-config.cmake`.

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/simulation-env.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/simulation-env.py
  @ONLY)
