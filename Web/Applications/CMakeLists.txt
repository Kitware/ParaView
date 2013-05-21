vtk_module_impl()
vtk_module_export("")

set(Apps_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(Apps_BINARY_DIR "${PARAVIEW_WWW_DIR}/apps")

set (apps DataProber
          LiveArticles
          WebFileViewer
          WebVisualizer)

set (all-copy-complete)

foreach (app ${apps})
  file(GLOB_RECURSE files "${Apps_SOURCE_DIR}/${app}/*.*")
  set (copy-complete "${CMAKE_CURRENT_BINARY_DIR}/${app}-copy-complete")
  add_custom_command(
    OUTPUT "${copy-complete}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
                                "${Apps_SOURCE_DIR}/${app}"
                                "${Apps_BINARY_DIR}/${app}"
    COMMAND ${CMAKE_COMMAND} -E touch ${copy-complete}
    COMMENT "Copying Web Application - '${app}'"
    DEPENDS ${files}
    )
  list(APPEND all-copy-complete "${copy-complete}")
endforeach()


add_custom_target(WebApplications ALL
  DEPENDS ${all-copy-complete})
