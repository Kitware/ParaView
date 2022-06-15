add_custom_target(DummyDocumentationCustomTarget
  COMMAND "${CMAKE_COMMAND}" "-DTARGET_DOC_DIR=${CMAKE_CURRENT_SOURCE_DIR}/Documentation" -P "${CMAKE_CURRENT_SOURCE_DIR}/Documentation/CreateDummyHtml.cmake"
)

set(table_of_contents
"<toc>
  <section title=\"My Elevation Filter Plugin\" ref=\"index.html\">
    <section title=\"Elevation Filter Documentation\" ref=\"index.html\"/>
    <section title=\"Irrelevant documentation\" ref=\"created.html\">
      <section title=\"Dummy Text File\" ref=\"dummy.txt\"/>
      <section title=\"Created File\" ref=\"created.html\"/>
    </section>
  </section>
</toc>")

paraview_add_plugin(ElevationFilter
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  DOCUMENTATION_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Documentation"
  DOCUMENTATION_ADD_PATTERNS
    "*.txt"
    "img/*.jpg"
    "js/*.js"
    "style/style.css"
  DOCUMENTATION_DEPENDENCIES DummyDocumentationCustomTarget
  DOCUMENTATION_TOC "${table_of_contents}"
  MODULES ElevationFilters
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/ElevationFilters/vtk.module")

option(BUILD_TESTING "Build Plugin Testing" OFF)
if (BUILD_TESTING AND BUILD_SHARED_LIBS)
  add_subdirectory(Testing)
endif()
