configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/index.html.in"
    "${CMAKE_CURRENT_BINARY_DIR}/index.html"
    @ONLY)
    
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/about.html.in"
    "${CMAKE_CURRENT_BINARY_DIR}/about.html"
    @ONLY)

# sanity check
if (NOT application_gui_xmls)
  message(AUTHOR_WARNING "application_gui_xmls is empty."
    "It must generally point to all the gui xmls in your application")
endif()

# generate htmls from the server manager xmls.
generate_htmls_from_xmls(proxy_html_outputs
  "${PARAVIEW_SERVERMANAGER_XMLS}"
  "${application_gui_xmls}"
  "${CMAKE_CURRENT_BINARY_DIR}")

# this function produces a ${CMAKE_CURRENT_BINARY_DIR}/paraview.qch file.
build_help_project(paraview
  DESTINATION_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  DOCUMENTATION_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
  TABLE_OF_CONTENTS
    "<toc>
       <section title=\"ParaView User Manual\" ref=\"index.html\">
         <section title=\"Introduction\" ref=\"Book/Book_Chapter_01_Intro.html\"/>
         <section title=\"Python Calculator\" ref=\"Book/Book_Chapter_06_PythonCalculator.html\"/>
         <section title=\"Display Data\" ref=\"Book/Book_Chapter_04_DisplayData.html\"/>
         <section title=\"Query Data\" ref=\"Book/Book_Chapter_06_findData.html\"/>
         <section title=\"Plugins\" ref=\"Book/Book_Chapter_16_Plugins.html\"/>
         <section title=\"Appendices\" ref=\"Book/Book_Chapter_17_Appendix.html\"/>
         <section title=\"Sources\" ref=\"Sources.html\"/>
         <section title=\"Filters\" ref=\"Filters.html\"/>
         <section title=\"Readers\" ref=\"Readers.html\"/>
         <section title=\"Writers\" ref=\"Writers.html\"/>
       </section>
     </toc>"
  FILEPATTERNS "*.html;*.css;*.png;*.jpg;Book/*.html;Book/*.css;Book/*.png;Book/*.jpg;images/*.jpg"
  DEPENDS "${proxy_html_outputs}"
  )

generate_qt_resource_from_files(
  "${CMAKE_CURRENT_BINARY_DIR}/paraview_documentation.qrc"
  "/ParaView/Documentation"
  "${CMAKE_CURRENT_BINARY_DIR}/paraview.qch")
set_source_files_properties(
  "${CMAKE_CURRENT_BINARY_DIR}/paraview_documentation.qrc"
  PROPERTIES OBJECT_DEPENDS
  "${CMAKE_CURRENT_BINARY_DIR}/paraview.qch")

QT4_ADD_RESOURCES(rcs_sources
  "${CMAKE_CURRENT_BINARY_DIR}/paraview_documentation.qrc")

add_library(vtkParaViewDocumentation STATIC
  ParaViewDocumentationInitializer.cxx
  ${rcs_sources})
target_link_libraries(vtkParaViewDocumentation ${QT_LIBRARIES}) 


IF (NOT VTK_INSTALL_NO_DEVELOPMENT)
  # We install the qch file only when installing development files since the
  # file in compiled into a library for run-time and hence need not be installed
  # otherwise.
  INSTALL(
      FILES ${CMAKE_CURRENT_BINARY_DIR}/paraview.qch
      DESTINATION ${VTK_INSTALL_DOC_DIR}
      COMPONENT Development)
ENDIF (NOT VTK_INSTALL_NO_DEVELOPMENT)
