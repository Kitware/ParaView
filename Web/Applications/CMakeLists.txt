vtk_module_impl()
vtk_module_export("")

set(Apps_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(Apps_BINARY_DIR "${PARAVIEW_WWW_DIR}/apps")

set (WEB_APPLICATIONS
  DataProber
  LiveArticles
  FileViewer
  Visualizer
  Parallel
)

# Create paraview/web directory
file(MAKE_DIRECTORY "${VTK_BUILD_PYTHON_MODULE_DIR}/paraview/web")

if (BUILD_TESTING)
  list(APPEND WEB_APPLICATIONS TestApp)
  file(MAKE_DIRECTORY "${VTK_BUILD_PYTHON_MODULE_DIR}/paraview/web/test")
endif()

set (WEB_APPS_DEPENDS)

foreach (_app ${WEB_APPLICATIONS})

  file(MAKE_DIRECTORY "${Apps_BINARY_DIR}/${_app}")

  copy_files_recursive(${Apps_SOURCE_DIR}/${_app}/www
    DESTINATION ${Apps_BINARY_DIR}/${_app}
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/www-${_app}-complete"
    REGEX "^(.*\\.*)$"
    LABEL "Copying Web files for ${_app}")

  copy_files_recursive(${Apps_SOURCE_DIR}/${_app}/server
    DESTINATION "${VTK_BUILD_PYTHON_MODULE_DIR}/paraview/web"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/py-${_app}-complete"
    REGEX "^(.*\\.py)$"
    LABEL "Copying Server files for ${_app}")

  list(APPEND WEB_APPS_DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/py-${_app}-complete"
    "${CMAKE_CURRENT_BINARY_DIR}/www-${_app}-complete")

  if (BUILD_TESTING)
    copy_files_recursive(${Apps_SOURCE_DIR}/${_app}/test
      DESTINATION "${VTK_BUILD_PYTHON_MODULE_DIR}/paraview/web/test"
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/test-${_app}-complete"
      REGEX "^(.*\\.py)$"
      LABEL "Copying Testing files for ${_app}")

    list(APPEND WEB_APPS_DEPENDS
      "${CMAKE_CURRENT_BINARY_DIR}/test-${_app}-complete")
  endif()

endforeach()

add_custom_target(WebApplications ALL DEPENDS
  ${WEB_APPS_DEPENDS}
)
