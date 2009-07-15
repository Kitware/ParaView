# +---------------------------------------------------------------------------+
# |                                                                           |
# |                            VisIt Relocation                               |
# |                                                                           |
# +---------------------------------------------------------------------------+
#
# Purpose:
#
# Generates the configuration variables:
#   VISIT_DATABASE_PLUGINS      -- Plugins found (file names) (list)
#   VISIT_LOCAL                 -- /path/to/localized/VisIt (see configureVisItBin target)
#
# Provides the targets:
#   configureVisItBin
#     Copy the VisIt libraries and database plugins (and windows dependencies)
#     into the project's binary directory.
#

set(_configureVisItDeps)

if(WIN32)
  set(VISIT_LOCAL "${ParaView_BINARY_DIR}/bin/${PLUGIN_BUILD_TYPE}")
else(WIN32)
  set(VISIT_LOCAL "${ParaView_BINARY_DIR}/bin")
endif(WIN32)

set (_visit_database_files)
set (_visit_avt_files)
set (_visit_third_party_files)

if (UNIX OR CYGWIN)
  # +------------------+
  # | Unix/Linux/Cygwin|
  # +------------------+
  # Collect the VisIt plugin's and libraries we will need at runtime.
  # Copy VisIt's libs.
  foreach (_dash_l ${VISIT_LIBS})
    set(_lib "lib${_dash_l}.so")
    set(_src "${VISIT_LIB_PATH}/${_lib}")
    
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E copy ${_src} "${VISIT_LOCAL}")
    set(_configureVisItDeps ${_configureVisItDeps} "${VISIT_LOCAL}/${_lib}")
    set(_visit_avt_files ${_visit_avt_files} "${VISIT_LOCAL}/${_lib}")
  endforeach (_dash_l)
  # Gather the database plugins that currently exist.
  execute_process(${CMAKE_COMMAND} -E make_directory "${VISIT_LOCAL}/databases")
  set(_configureVisItDeps ${_configureVisItDeps} "${VISIT_LOCAL}/databases")
  file(GLOB 
    VISIT_DATABASE_PLUGINS
    "${VISIT_PLUGIN_BIN}/databases/*.so")
  foreach (_dbPath ${VISIT_DATABASE_PLUGINS})
    get_filename_component(_db ${_dbPath} NAME)
    add_custom_command(
      OUTPUT "${VISIT_LOCAL}/databases/${_db}"
      COMMAND ${CMAKE_COMMAND}
      ARGS -E copy ${_dbPath} "${VISIT_LOCAL}/databases"
      DEPENDS ${_dbPath}
      COMMENT "Copying ${_db} to local VisIt databases.")
    set(_configureVisItDeps ${_configureVisItDeps} ${VISIT_LOCAL}/databases/${_db})
    set(_visit_database_files ${_visit_database_files} ${VISIT_LOCAL}/databases/${_db})
  endforeach (_dbPath)
  # Gather VisIt's third party deps.
  # We don't do this on Linux because it makes more sense for the user to have
  # the installed and configured to suite their own needs.
else (UNIX OR CYGWIN)
  # +------------------+
  # |     Windows      |
  # +------------------+
  # Collect the VisIt plugin's and libraries we will need at runtime.
  # Copy VisIt's libs.
  foreach (_dash_l ${VISIT_LIBS})
    # dll's
    set(_dlib "${_dash_l}.dll")
    set(_dlsrc "${VISIT_BIN_PATH}/${PLUGIN_BUILD_TYPE}/${_dlib}")
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E copy ${_dlsrc} "${VISIT_LOCAL}")
  #  message("_dlsrc: ${_dlsrc} to ${VISIT_LOCAL}")
    set(_configureVisItDeps ${_configureVisItDeps} "${VISIT_LOCAL}/${_dlib}")
    set(_visit_avt_files ${_visit_avt_files} "${VISIT_LOCAL}/${_dlib}")
    # lib's
  #  set(_slib "${_dash_l}.lib")
  #  set(_slsrc "${VISIT_LIB_PATH}/${PLUGIN_BUILD_TYPE}/${_slib}")
  #  execute_process(
  #    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_slsrc} "${VISIT_LOCAL}")
  #  message("_slsrc: ${_slsrc} to ${VISIT_LOCAL}")
  #  set(_configureVisItDeps ${_configureVisItDeps} "${VISIT_LOCAL}/${_slib}")
  #  set(_visit_avt_files ${_visit_avt_files} "${VISIT_LOCAL}/${_slib}")
  endforeach (_dash_l)
  # Gather the database plugins that currently exist.
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E make_directory "${VISIT_LOCAL}/databases")
  #set(_configureVisItDeps ${_configureVisItDeps} "${VISIT_LOCAL}/databases")
  file(GLOB 
    VISIT_DATABASE_PLUGINS
    "${VISIT_PLUGIN_BIN}/databases/*.dll")
  foreach (_dbPath ${VISIT_DATABASE_PLUGINS})
    get_filename_component(_db ${_dbPath} NAME)
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E copy ${_dbPath} "${VISIT_LOCAL}/databases")
  #  message("_dlsrc: ${_dbPath} to ${VISIT_LOCAL}")
  #  set(_configureVisItDeps ${_configureVisItDeps} ${VISIT_LOCAL}/databases/${_db})
    set(_visit_database_files ${_visit_database_files} ${VISIT_LOCAL}/databases/${_db})
  endforeach (_dbPath)
  # Gather VisIt's third party deps, They come pre-built in the Windows project.
  # dll's
  file(GLOB 
    _libs
    "${VISIT_BIN_PATH}/Thirdparty/*.dll")
  foreach (_lib ${_libs})
    get_filename_component(_fn ${_lib} NAME)
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E copy ${_lib} "${VISIT_LOCAL}")
  #  message("_dlsrc: ${_lib} to ${VISIT_LOCAL}")
     set(_visit_third_party_files ${_visit_third_party_files} ${VISIT_LOCAL}/${_fn})
  endforeach (_lib)
  # lib's
  #file(GLOB 
  #  _libs
  #  "${VISIT_LIB_PATH}/Thirdparty/*.lib")
  #foreach (_lib ${_libs})
    #get_filename_component(_fn ${_lib} NAME)
    #execute_process(
    #  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_lib} "${VISIT_LOCAL}")
    #message("_dlsrc: ${_lib} to ${VISIT_LOCAL}")
    #set(_configureVisItDeps ${_configureVisItDeps} ${VISIT_LOCAL}/${_fn})
  #endforeach (_lib)
endif (UNIX OR CYGWIN)

#
#add_custom_target(configureVisItBin DEPENDS ${_configureVisItDeps})
set (VISIT_FILES_TO_INSTALL_AVT "${_visit_avt_files}" CACHE INTERNAL "Visit AVT Files")
set (VISIT_FILES_TO_INSTALL_DATABASES "${_visit_database_files}" CACHE INTERNAL "Visit Database Files")
set (VISIT_FILES_TO_INSTALL_THIRD_PARTY "${_visit_third_party_files}" CACHE INTERNAL "Visit Third Party Files")
