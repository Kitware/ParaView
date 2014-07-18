vtk_module_impl()
vtk_module_export("")

# Everything under Ext is built in www/ext/
# Everything under other directories is placed in www/lib/js or www/lib/css

# =================================================================
# NV charts
# =================================================================

set(nvcharts_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/NVCharts")
set(nvcharts_binary_dir_js "${PARAVIEW_WWW_DIR}/lib/js")

copy_files_recursive(${nvcharts_source_dir}
  DESTINATION ${nvcharts_binary_dir_js}
  REGEX "^(.*\\.js)$"
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/nvcharts-js-copy-complete"
  LABEL "Copying NVCharts JavaScript files")

# =================================================================
# Rickshaw charts
# =================================================================

set(rickshawcharts_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/RickshawCharts")
set(rickshawcharts_binary_dir_js "${PARAVIEW_WWW_DIR}/lib/js")

copy_files_recursive(${rickshawcharts_source_dir}
  DESTINATION ${rickshawcharts_binary_dir_js}
  REGEX "^(.*\\.js)$"
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/rickshawcharts-js-copy-complete"
  LABEL "Copying RickshawCharts JavaScript files")

# =================================================================
# Pipeline browser
# =================================================================

set(paraview_pipeline_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/PipelineBrowser")
set(paraview_pipeline_binary_dir_js "${PARAVIEW_WWW_DIR}/lib/js")
set(paraview_pipeline_binary_dir_css "${PARAVIEW_WWW_DIR}/lib/css")

copy_files_recursive(${paraview_pipeline_source_dir}
  DESTINATION ${paraview_pipeline_binary_dir_js}
  REGEX "^(.*\\.js)$"
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/paraview_pipeline-js-copy-complete"
  LABEL "Copying ParaView Pipeline JavaScript files")

copy_files_recursive(${paraview_pipeline_source_dir}
  DESTINATION ${paraview_pipeline_binary_dir_css}
  REGEX "^(.*\\.js)$" EXCLUDE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/paraview_pipeline-css-copy-complete"
  LABEL "Copying ParaView Pipeline CSS files")

# =================================================================
# New Pipeline components
# =================================================================

set(paraview_pipeline2_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/Pipeline")

copy_files_recursive(${paraview_pipeline2_source_dir}
  DESTINATION ${paraview_pipeline_binary_dir_js}
  REGEX "^(.*\\.js)$"
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/paraview_pipeline2-js-copy-complete"
  LABEL "Copying ParaView Pipeline JavaScript files")

copy_files_recursive(${paraview_pipeline2_source_dir}
  DESTINATION ${paraview_pipeline_binary_dir_css}
  REGEX "^(.*\\.js)$" EXCLUDE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/paraview_pipeline2-css-copy-complete"
  LABEL "Copying ParaView Pipeline CSS files")

# =================================================================
# Toolbars
# =================================================================

set(paraview_toolbars_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/Toolbars")
set(paraview_toolbars_binary_dir_js "${CMAKE_BINARY_DIR}/www/lib/js")
set(paraview_toolbars_binary_dir_css "${CMAKE_BINARY_DIR}/www/lib/css")

copy_files_recursive(${paraview_toolbars_source_dir}
  DESTINATION ${paraview_toolbars_binary_dir_js}
  REGEX "^(.*\\.js)$"
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/paraview_toolbars-js-copy-complete"
  LABEL "Copying ParaView Toolbars JavaScript files")

copy_files_recursive(${paraview_toolbars_source_dir}
  DESTINATION ${paraview_toolbars_binary_dir_css}
  REGEX "^(.*\\.js)$" EXCLUDE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/paraview_toolbars-css-copy-complete"
  LABEL "Copying ParaView Toolbars CSS files")

# =================================================================
# Logos
# =================================================================

set(paraview_logo_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/Cinema")
set(paraview_logo_binary_dir "${CMAKE_BINARY_DIR}/www/ext/pv")

copy_files_recursive(${paraview_logo_source_dir}
  DESTINATION ${paraview_logo_binary_dir}
  REGEX "^(.*\\.png)$"
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/paraview_logo-copy-complete"
  LABEL "Copying ParaView Logos files")


# =================================================================
# Dependency
# =================================================================

add_custom_target(ParaViewWebWidgets ALL DEPENDS
  "${CMAKE_CURRENT_BINARY_DIR}/nvcharts-js-copy-complete"
  "${CMAKE_CURRENT_BINARY_DIR}/rickshawcharts-js-copy-complete"
  "${CMAKE_CURRENT_BINARY_DIR}/paraview_pipeline-js-copy-complete"
  "${CMAKE_CURRENT_BINARY_DIR}/paraview_pipeline-css-copy-complete"
  "${CMAKE_CURRENT_BINARY_DIR}/paraview_pipeline2-js-copy-complete"
  "${CMAKE_CURRENT_BINARY_DIR}/paraview_pipeline2-css-copy-complete"
  "${CMAKE_CURRENT_BINARY_DIR}/paraview_toolbars-js-copy-complete"
  "${CMAKE_CURRENT_BINARY_DIR}/paraview_toolbars-css-copy-complete"
  "${CMAKE_CURRENT_BINARY_DIR}/paraview_logo-copy-complete"
  )
