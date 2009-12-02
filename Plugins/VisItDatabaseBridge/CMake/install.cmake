# +---------------------------------------------------------------------------+
# |                                                                           |
# |                                 Install                                   |
# |                                                                           |
# +---------------------------------------------------------------------------+

SET (visit_install_dir ${PV_INSTALL_LIB_DIR})
IF (WIN32)
  SET (visit_install_dir ${PV_INSTALL_BIN_DIR})
ENDIF(WIN32)

install(FILES ${VISIT_FILES_TO_INSTALL_AVT} 
  DESTINATION "${visit_install_dir}"
  COMPONENT Runtime)
 
install(FILES ${VISIT_FILES_TO_INSTALL_THIRD_PARTY}
  DESTINATION "${visit_install_dir}"
  COMPONENT Runtime)

#install databases
install(FILES ${VISIT_FILES_TO_INSTALL_DATABASES}
  DESTINATION "${visit_install_dir}/databases"
  COMPONENT Runtime)

#set(_libExts ".a" ".so" ".lib" ".dll")
#foreach(_ext ${_libExts})
#  file(GLOB _tmpLibs "${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_BUILD_TYPE}/*${_ext}")
#  file(GLOB _tmpDatabases "${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_BUILD_TYPE}/databases/*${_ext}")
#  set(_libs ${_libs} ${_tmpLibs})
#  set(_databases ${_databases} ${_tmpDatabases})
#endforeach(_ext)
#
#install(FILES ${_libs} DESTINATION ${CMAKE_INSTALL_PREFIX})
#install(FILES ${_databases} DESTINATION ${CMAKE_INSTALL_PREFIX}/databases)

