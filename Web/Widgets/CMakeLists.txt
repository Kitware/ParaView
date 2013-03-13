vtk_module_impl()
vtk_module_export("")

# Everything under Ext is built in www/js/ext/ or www/css/ext/
# Everything under other directories is placed in www/js/lib or www/css/lib/

set(ext_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/Ext")
set(ext_binary_dir_js "${CMAKE_BINARY_DIR}/www/js/ext")
set(ext_binary_dir_css "${CMAKE_BINARY_DIR}/www/css/ext")

copy_files_recursive(${ext_source_dir}
  DESTINATION ${ext_binary_dir_js}
  REGEX "^(.*\\.js)$"
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ext-js-copy-complete"
  LABEL "Copying widgets JavaScript files")

copy_files_recursive(${ext_source_dir}
  DESTINATION ${ext_binary_dir_css}
  REGEX "^(.*\\.js)$" EXCLUDE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ext-css-copy-complete"
  LABEL "Copying widgets css/image files")

set(nvcharts_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/NVCharts")
set(nvcharts_binary_dir_js "${CMAKE_BINARY_DIR}/www/js/lib")
copy_files_recursive(${nvcharts_source_dir}
  DESTINATION ${nvcharts_binary_dir_js}
  REGEX "^(.*\\.js)$"
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/nvcharts-js-copy-complete"
  LABEL "Copying NVCharts JavaScript files")


set(rickshawcharts_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/RickshawCharts")
set(rickshawcharts_binary_dir_js "${CMAKE_BINARY_DIR}/www/js/lib")
copy_files_recursive(${rickshawcharts_source_dir}
  DESTINATION ${rickshawcharts_binary_dir_js}
  REGEX "^(.*\\.js)$"
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/rickshawcharts-js-copy-complete"
  LABEL "Copying RickshawCharts JavaScript files")

add_custom_target(ParaViewWebWidgets ALL DEPENDS
  "${CMAKE_CURRENT_BINARY_DIR}/ext-css-copy-complete"
  "${CMAKE_CURRENT_BINARY_DIR}/ext-js-copy-complete"
  "${CMAKE_CURRENT_BINARY_DIR}/nvcharts-js-copy-complete"
  "${CMAKE_CURRENT_BINARY_DIR}/rickshawcharts-js-copy-complete"
  )
