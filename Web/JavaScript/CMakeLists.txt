vtk_module_impl()
vtk_module_export("")

find_package(PythonInterp REQUIRED)

set(args)
list(APPEND args "-b" "${CMAKE_CURRENT_SOURCE_DIR}/banner.js.in")
list(APPEND args "-v" "v2.0")

set(lib_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/Lib")
set(lib_binary_dir "${CMAKE_BINARY_DIR}/www/lib/js")

set(lib_js_min_files
  "${lib_source_dir}/paraview.launcher.js"
  "${lib_source_dir}/paraview.connect.js"
  "${lib_source_dir}/paraview.viewport.js"
  "${lib_source_dir}/paraview.viewport.image.js"
  "${lib_source_dir}/paraview.viewport.webgl.js"
  "${lib_source_dir}/paraview.statistics.js"
  )

list(APPEND args "-i")

foreach(_file ${lib_js_min_files})
  list(APPEND args "${_file}")
endforeach()

set(paraview_all_min_js "${lib_binary_dir}/paraview-all.min.js")
set(paraview_all_js "${lib_binary_dir}/paraview-all.js")

list(APPEND args "-o" ${paraview_all_min_js})
list(APPEND args "-m" ${paraview_all_js})

add_custom_command(OUTPUT ${paraview_all_min_js} ${paraview_all_js}
  COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/Minimizer/build.py" ${args}
  DEPENDS ${lib_js_min_files})

# Copy Ext js files
set(ext_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/Ext")
set(ext_binary_dir "${CMAKE_BINARY_DIR}/www/ext/js-core")

copy_files_recursive(${ext_source_dir}
  DESTINATION ${ext_binary_dir}
  REGEX "^(.*\\.js)$"
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ext-js-copy-complete"
  LABEL "Copying JavaScript files")

add_custom_target(vtkParaViewWebJavaScript ALL DEPENDS
  ${paraview_all_min_js}
  "${CMAKE_CURRENT_BINARY_DIR}/ext-js-copy-complete")

install(DIRECTORY ${ext_binary_dir}
  DESTINATION ${VTK_INSTALL_LIBRARY_DIR}/www/ext/js
  COMPONENT Runtime)

install(DIRECTORY ${lib_binary_dir}
  DESTINATION ${VTK_INSTALL_LIBRARY_DIR}/www/lib/js
  COMPONENT Runtime)
