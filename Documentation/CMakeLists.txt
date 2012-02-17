configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/index.html.in"
    "${CMAKE_CURRENT_BINARY_DIR}/index.html"
    @ONLY)
    
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/about.html.in"
    "${CMAKE_CURRENT_BINARY_DIR}/about.html"
    @ONLY)

set (gui_xmls
  ${ParaView_SOURCE_DIR}/Applications/ParaView/ParaViewSources.xml
  ${ParaView_SOURCE_DIR}/Applications/ParaView/ParaViewFilters.xml
  ${ParaView_SOURCE_DIR}/Applications/ParaView/ParaViewReaders.xml
  ${ParaView_SOURCE_DIR}/Applications/ParaView/ParaViewWriters.xml)

# generate htmls from the server manager xmls.
generate_htmls_from_xmls(proxy_html_outputs
  "${PARAVIEW_SERVERMANAGER_XMLS}"
  "${gui_xmls}"
  "${CMAKE_CURRENT_BINARY_DIR}")

# this function produces a ${CMAKE_CURRENT_BINARY_DIR}/paraview.qch file.
build_help_project(paraview
  DESTINATION_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  DOCUMENTATION_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
  TABLE_OF_CONTENTS
    "<toc>
       <section title=\"ParaView User Manual\" ref=\"index.html\">
         <section title=\"Introduction\" ref=\"Book/Book_Chapter1.html\"/>
         <section title=\"Python Calculator\" ref=\"Book/Book_PythonCalculator.html\"/>
         <section title=\"Query Data\" ref=\"Book/Book_Chapter6.html\"/>
         <section title=\"Plugins\" ref=\"Book/Book_Chapter16.html\"/>
         <section title=\"Appendices\" ref=\"Book/Book_Chapter17.html\"/>
         <section title=\"Sources\" ref=\"Sources.html\"/>
         <section title=\"Filters\" ref=\"Filters.html\"/>
         <section title=\"Readers\" ref=\"Readers.html\"/>
         <section title=\"Writers\" ref=\"Writers.html\"/>
       </section>
     </toc>"
  FILEPATTERNS "*.html;*.css;*.png;*.jpg;Book/*.html;Book/*.css;Book/*.png;Book/*.jpg;images/*.jpg"
  DEPENDS "${proxy_html_outputs}"
  )

ADD_CUSTOM_TARGET (ParaViewOnlineHelp
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/paraview.qch)


SET(USERS_GUIDE_FILE "ParaView User's Guide ${PARAVIEW_VERSION}.pdf")
FIND_FILE(PARAVIEW_USERS_GUIDE ${USERS_GUIDE_FILE})
MARK_AS_ADVANCED(PARAVIEW_USERS_GUIDE)

IF (PARAVIEW_USERS_GUIDE AND EXISTS "${PARAVIEW_USERS_GUIDE}")
  INSTALL(
      FILES ${PARAVIEW_USERS_GUIDE}
      DESTINATION ${PV_INSTALL_DOC_DIR}
      COMPONENT Runtime)
ENDIF()

# Development
IF (NOT PV_INSTALL_NO_DEVELOPMENT)
  INSTALL(
      FILES ${CMAKE_CURRENT_BINARY_DIR}/paraview.qch
      DESTINATION ${PV_INSTALL_DOC_DIR} 
      COMPONENT Development)
ENDIF (NOT PV_INSTALL_NO_DEVELOPMENT)
