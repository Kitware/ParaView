vtk_module_impl()
vtk_module_export("")

find_package(JSDuck REQUIRED)

set(docs_source_dir "${CMAKE_CURRENT_SOURCE_DIR}")
set(docs_binary_dir "${CMAKE_CURRENT_BINARY_DIR}/web-content")
set(docs_tmp_dir "${CMAKE_CURRENT_BINARY_DIR}/tmp")
set(JAVASCRIPT_LIB_DIR "${CMAKE_SOURCE_DIR}/VTK/Web/JavaScript")
set(JAVASCRIPT_WIDGETS_DIR "${CMAKE_SOURCE_DIR}/Web/Widgets")
set(DOC_OUTPUT_DIR "${docs_binary_dir}")
set(DOC_MERGE_GUIDES_DIR "${CMAKE_CURRENT_BINARY_DIR}/guide-merged")

# favicon.ico"
configure_file("${docs_source_dir}/images/logo/favicon.ico"
  "${docs_tmp_dir}/favicon.ico" COPYONLY)

# custom.css
configure_file("${docs_source_dir}/custom.css"
  "${docs_tmp_dir}/custom.css" COPYONLY)

# images
configure_file("${docs_source_dir}/images/logo/logo.png"
  "${docs_tmp_dir}/resources/images/logo.png" COPYONLY)

configure_file("${docs_source_dir}/images/web/PVW_Slide.png"
  "${docs_tmp_dir}/images/PVW_Slide.png" COPYONLY)

configure_file("${docs_source_dir}/images/compatible/compatible_chrome.gif"
  "${docs_tmp_dir}/images/compatible_chrome.gif" COPYONLY)
configure_file("${docs_source_dir}/images/compatible/compatible_firefox.gif"
  "${docs_tmp_dir}/images/compatible_firefox.gif" COPYONLY)
configure_file("${docs_source_dir}/images/compatible/compatible_ie.gif"
  "${docs_tmp_dir}/images/compatible_ie.gif" COPYONLY)
configure_file("${docs_source_dir}/images/compatible/compatible_opera.gif"
  "${docs_tmp_dir}/images/compatible_opera.gif" COPYONLY)
configure_file("${docs_source_dir}/images/compatible/compatible_safari.gif"
  "${docs_tmp_dir}/images/compatible_safari.gif" COPYONLY)

add_custom_target(webcontent DEPENDS ${_web_content_files})
add_custom_target(pvw ALL DEPENDS ${_paraview_all_min_js} webcontent)

configure_file(${docs_source_dir}/config.json.in
   "${CMAKE_CURRENT_BINARY_DIR}/config.json")

configure_file(${docs_source_dir}/guides.json
   "${DOC_MERGE_GUIDES_DIR}/guides.json" COPYONLY)

file (
  COPY
  "${CMAKE_CURRENT_SOURCE_DIR}/guides"
  DESTINATION
  "${DOC_MERGE_GUIDES_DIR}"
)

# setup up dependencies ...
set(doc_input_files "${CMAKE_CURRENT_BINARY_DIR}/config.json")
file(GLOB_RECURSE _doc_files ${docs_source_dir} "*.*")
list(APPEND doc_input_files ${_doc_files})

set(args "--config=${CMAKE_CURRENT_BINARY_DIR}/config.json")
set(jsduck_stamp "${CMAKE_CURRENT_BINARY_DIR}/jsduck.stamp")

add_custom_command(OUTPUT ${jsduck_stamp}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${docs_tmp_dir}
  COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/jsduck.stamp"
  COMMAND ${JSDUCK_EXECUTABLE} ${args}
  DEPENDS ${doc_input_files})

add_custom_target(ParaViewWebDoc
  DEPENDS ${jsduck_stamp}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${docs_tmp_dir} ${docs_binary_dir}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${docs_binary_dir} "${PARAVIEW_WWW_DIR}/js-doc"
)

add_dependencies(ParaViewDoc ParaViewWebDoc)

set_directory_properties(PROPERTIES
  ADDITIONAL_MAKE_CLEAN_FILES "${docs_binary_dir}")
