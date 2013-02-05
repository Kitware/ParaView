vtk_module_impl()
vtk_module_export("")

set(Apps_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(Apps_BINARY_DIR "${CMAKE_BINARY_DIR}/www/apps")

file(GLOB_RECURSE files
  RELATIVE ${Apps_SOURCE_DIR}
  "${Apps_SOURCE_DIR}/LiveArticles/*.*"
  "${Apps_SOURCE_DIR}/WebFileViewer/*.*"
  "${Apps_SOURCE_DIR}/WebVisualizer/*.*")

foreach(_file ${files})
  configure_file("${Apps_SOURCE_DIR}/${_file}" "${Apps_BINARY_DIR}/${_file}"
    COPYONLY)
endforeach()

install(DIRECTORY ${Apps_BINARY_DIR}
  DESTINATION ${VTK_INSTALL_LIBRARY_DIR}/www
  COMPONENT Runtime)
