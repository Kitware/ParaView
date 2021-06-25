option(ParaView_DEBUG_PLUGINS "Debug plugin logic in ParaView" OFF)
mark_as_advanced(ParaView_DEBUG_PLUGINS)

set(_paraview_plugin_log)

include(CMakeDependentOption)
cmake_dependent_option(ParaView_DEBUG_PLUGINS_ALL "Enable all debugging" OFF
  "ParaView_DEBUG_PLUGINS" OFF)
mark_as_advanced(ParaView_DEBUG_PLUGINS_ALL)

if (ParaView_DEBUG_PLUGINS_ALL)
  set(_paraview_plugin_log "ALL")
else ()
  set(_builtin_domains
    building
    plugin)
  foreach (_domain IN LISTS _builtin_domains _debug_domains)
    cmake_dependent_option("ParaView_DEBUG_PLUGINS_${_domain}" "Enable debugging of ${_domain} logic" OFF
      "ParaView_DEBUG_PLUGINS" OFF)
    mark_as_advanced("ParaView_DEBUG_PLUGINS_${_domain}")
    if (ParaView_DEBUG_PLUGINS_${_domain})
      list(APPEND _paraview_plugin_log
        "${_domain}")
    endif ()
  endforeach ()
  unset(_domain)
  unset(_builtin_domains)
endif ()
