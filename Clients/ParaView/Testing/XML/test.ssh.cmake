
message(STATUS
  "With PARAVIEW_SSH_SERVERS_TESTING, to have the SSH Server tests pass, make "
  "sure that the machine running the test has a SSH server running on port "
  "2222, its own ssh public key as an authorized_keys, that 127.0.0.1 is in "
  "the known_hosts file and that /usr/bin/xterm is available. In case of "
  "failing tests, pvserver logs are available in "
  "${CMAKE_CURRENT_BINARY_DIR}/sshServer-*.log")

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/server.sh.in"
  "${CMAKE_CURRENT_BINARY_DIR}/server.sh" @ONLY)
configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/server_rc.sh.in"
  "${CMAKE_CURRENT_BINARY_DIR}/server_rc.sh" @ONLY)

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/sshServers.pvsc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/sshServers.pvsc" @ONLY)

set(ssh_server_tests
  SimpleSSHServer
  SimpleSSHServerAskPass
  SimpleSSHServerTermExec
  SimpleRCSSHServer
  SSHServerPortForwarding
  RCSSHServerPortForwarding
  RCSSHServerPortForwardingOption
  )

foreach(tname IN LISTS ssh_server_tests)
  configure_file (
    "${tname}.xml.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${tname}.xml" @ONLY)

  list(APPEND TESTS_WITHOUT_BASELINES
    ${CMAKE_CURRENT_BINARY_DIR}/${tname}.xml)
  set(${tname}_DISABLE_CS TRUE)
  set(${tname}_DISABLE_CRS TRUE)
  # The SSH tests should not be run in parallel to avoid port collision
  set(${tname}_FORCE_SERIAL TRUE)
endforeach()
