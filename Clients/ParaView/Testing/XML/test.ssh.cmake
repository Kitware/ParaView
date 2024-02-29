
message(STATUS
  "With PARAVIEW_SSH_SERVERS_TESTING, to have the SSH Server tests pass, make sure that this machine has its own ssh public key as an authorized_keys and that 127.0.0.1 is in the known_hosts file, /usr/bin/xterm is available. In case of failing tests, pvserver logs are available in ${CMAKE_CURRENT_BINARY_DIR}/sshServer.log")

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/server.sh.in"
  "${CMAKE_CURRENT_BINARY_DIR}/tmp/server.sh" @ONLY)
configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/server_rc.sh.in"
  "${CMAKE_CURRENT_BINARY_DIR}/tmp/server_rc.sh" @ONLY)

file(
  COPY ${CMAKE_CURRENT_BINARY_DIR}/tmp/server.sh
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  )

file(
  COPY ${CMAKE_CURRENT_BINARY_DIR}/tmp/server_rc.sh
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  )

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
