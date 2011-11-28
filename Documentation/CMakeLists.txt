# Copy all online help files from current directory over to the binary directory.
# These do not include any server manager documentation htmls.
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR})

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/index.html.in"
    "${CMAKE_CURRENT_BINARY_DIR}/index.html"
    @ONLY)
    
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/about.html.in"
    "${CMAKE_CURRENT_BINARY_DIR}/about.html"
    @ONLY)

FIND_PROGRAM(QT_HELP_GENERATOR
  qhelpgenerator
  PATHS "${QT_BINARY_DIR}"
  DOC "qhelpgenerator used to compile Qt help project files")

FIND_PROGRAM(QT_XMLPATTERNS_EXECUTABLE
  xmlpatterns
  PATHS "${QT_BINARY_DIR}"
  DOC "xmlpatterns used to generate html from Proxy documentation.")

MARK_AS_ADVANCED(QT_HELP_GENERATOR QT_XMLPATTERNS_EXECUTABLE)

set (gui_xmls
  ${ParaView_SOURCE_DIR}/Applications/ParaView/ParaViewSources.xml
  ${ParaView_SOURCE_DIR}/Applications/ParaView/ParaViewFilters.xml
  ${ParaView_SOURCE_DIR}/Applications/ParaView/ParaViewReaders.xml
  ${ParaView_SOURCE_DIR}/Applications/ParaView/ParaViewWriters.xml)

# generate htmls from the server manager xmls.
generate_htmls_from_xmls(proxy_html_outputs
  "${QT_XMLPATTERNS_EXECUTABLE}"
  "${PARAVIEW_SERVERMANAGER_XMLS}"
  "${gui_xmls}"
  "${CMAKE_CURRENT_BINARY_DIR}")

ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/paraview.qch
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/paraview.qhp
         ${proxy_html_outputs}
  COMMAND ${CMAKE_COMMAND}
          -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/paraview.qhp"
          "${CMAKE_CURRENT_BINARY_DIR}/paraview.qhp"
  COMMAND ${QT_HELP_GENERATOR}
          ${CMAKE_CURRENT_BINARY_DIR}/paraview.qhp
          -o ${CMAKE_CURRENT_BINARY_DIR}/paraview.qch
  COMMENT "Compiling Qt help project paraview.qhp"
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
