## Add zSpace plugin to CI

The zSpace ParaView plugin does not need to search for zSpace Core compatibility headers and libraries
at configuration time anymore (when ZSPACE_USE_COMPAT_SDK is set to ON, which is by default).
Since this plugin does not have any compile-time external dependencies, it has been to the CI to test that it builds correctly.
