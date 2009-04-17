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
set(VISIT_LOCAL "${PROJECT_BINARY_DIR}/${PLUGIN_BUILD_TYPE}")

if (UNIX OR CYGWIN)
  # +------------------+
  # | Unix/Linux/Cygwin|
  # +------------------+
  # Collect the VisIt plugin's and libraries we will need at runtime.
  # Copy VisIt's libs.
  foreach (_dash_l ${VISIT_LIBS})
    set(_lib "lib${_dash_l}.so")
    set(_src "${VISIT_LIB_PATH}/${_lib}")
    add_custom_command(
      OUTPUT "${VISIT_LOCAL}/${_lib}"
      COMMAND ${CMAKE_COMMAND}
      ARGS -E copy ${_src} "${VISIT_LOCAL}"
      DEPENDS ${_src}
      COMMENT "Copying ${_lib} to local VisIt.")
    set(_configureVisItDeps ${_configureVisItDeps} "${VISIT_LOCAL}/${_lib}")
  endforeach (_dash_l)
  # Gather the database plugins that currently exist.
  add_custom_command(
    OUTPUT "${VISIT_LOCAL}/databases"
    COMMAND ${CMAKE_COMMAND}
    ARGS -E make_directory "${VISIT_LOCAL}/databases"
    COMMENT "Making databases directory.")
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
    add_custom_command(
      OUTPUT "${VISIT_LOCAL}/${_dlib}"
      COMMAND ${CMAKE_COMMAND}
      ARGS -E copy ${_dlsrc} "${VISIT_LOCAL}"
      DEPENDS ${_dlsrc}
      COMMENT "Copying ${_dlib} to local VisIt.")
    set(_configureVisItDeps ${_configureVisItDeps} "${VISIT_LOCAL}/${_dlib}")
    # lib's
    set(_slib "${_dash_l}.lib")
    set(_slsrc "${VISIT_LIB_PATH}/${PLUGIN_BUILD_TYPE}/${_slib}")
    add_custom_command(
      OUTPUT "${VISIT_LOCAL}/${_slib}"
      COMMAND ${CMAKE_COMMAND}
      ARGS -E copy ${_slsrc} "${VISIT_LOCAL}"
      DEPENDS ${_slsrc}
      COMMENT "Copying ${_slib} to local VisIt.")
    set(_configureVisItDeps ${_configureVisItDeps} "${VISIT_LOCAL}/${_slib}")
  endforeach (_dash_l)
  # Gather the database plugins that currently exist.
  add_custom_command(
    OUTPUT "${VISIT_LOCAL}/databases"
    COMMAND ${CMAKE_COMMAND}
    ARGS -E make_directory "${VISIT_LOCAL}/databases"
    COMMENT "Making databases directory.")
  set(_configureVisItDeps ${_configureVisItDeps} "${VISIT_LOCAL}/databases")
  file(GLOB 
    VISIT_DATABASE_PLUGINS
    "${VISIT_PLUGIN_BIN}/databases/*.dll")
  foreach (_dbPath ${VISIT_DATABASE_PLUGINS})
    get_filename_component(_db ${_dbPath} NAME)
    add_custom_command(
      OUTPUT "${VISIT_LOCAL}/databases/${_db}"
      COMMAND ${CMAKE_COMMAND}
      ARGS -E copy ${_dbPath} "${VISIT_LOCAL}/databases"
      DEPENDS ${_dbPath}
      COMMENT "Copying ${_db} to local VisIt databases.")
    set(_configureVisItDeps ${_configureVisItDeps} ${VISIT_LOCAL}/databases/${_db})
  endforeach (_dbPath)
  # Gather VisIt's third party deps, They come pre-built in the Windows project.
  # dll's
  file(GLOB 
    _libs
    "${VISIT_BIN_PATH}/Thirdparty/*.dll")
  foreach (_lib ${_libs})
    get_filename_component(_fn ${_lib} NAME)
    add_custom_command(
      OUTPUT "${VISIT_LOCAL}/${_fn}"
      COMMAND ${CMAKE_COMMAND}
      ARGS -E copy ${_lib} "${VISIT_LOCAL}"
      DEPENDS ${_lib}
      COMMENT "Copying ${_fn} to local VisIt.")
    set(_configureVisItDeps ${_configureVisItDeps} ${VISIT_LOCAL}/${_fn})
  endforeach (_lib)
  # lib's
  file(GLOB 
    _libs
    "${VISIT_LIB_PATH}/Thirdparty/*.lib")
  foreach (_lib ${_libs})
    get_filename_component(_fn ${_lib} NAME)
    add_custom_command(
      OUTPUT "${VISIT_LOCAL}/${_fn}"
      COMMAND ${CMAKE_COMMAND}
      ARGS -E copy ${_lib} "${VISIT_LOCAL}"
      DEPENDS ${_lib}
      COMMENT "Copying ${_fn} to local VisIt.")
    set(_configureVisItDeps ${_configureVisItDeps} ${VISIT_LOCAL}/${_fn})
  endforeach (_lib)
endif (UNIX OR CYGWIN)

#
add_custom_target(configureVisItBin DEPENDS ${_configureVisItDeps})
