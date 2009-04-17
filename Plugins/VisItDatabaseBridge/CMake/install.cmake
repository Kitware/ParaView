# +---------------------------------------------------------------------------+
# |                                                                           |
# |                                 Install                                   |
# |                                                                           |
# +---------------------------------------------------------------------------+

set(_libExts ".a" ".so" ".lib" ".dll")
foreach(_ext ${_libExts})
  file(GLOB _tmpLibs "${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_BUILD_TYPE}/*${_ext}")
  file(GLOB _tmpDatabases "${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_BUILD_TYPE}/databases/*${_ext}")
  set(_libs ${_libs} ${_tmpLibs})
  set(_databases ${_databases} ${_tmpDatabases})
endforeach(_ext)

install(FILES ${_libs} DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES ${_databases} DESTINATION ${CMAKE_INSTALL_PREFIX}/databases)
