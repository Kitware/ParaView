# Copy all online help files from current directory over to the binary directory.
# These do not include any server manager documentation htmls.
file(GLOB PV_DOCUMENTATION_HTML_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/[^.]*.html
  ${CMAKE_CURRENT_SOURCE_DIR}/Book/[^.]*.html
  )
  
file(GLOB PV_DOCUMENTATION_PNG_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/[^.]*.png
  ${CMAKE_CURRENT_SOURCE_DIR}/Book/[^.]*.png
  )

file(GLOB PV_DOCUMENTATION_JPG_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/[^.]*.jpg
  ${CMAKE_CURRENT_SOURCE_DIR}/Book/[^.]*.jpg
  )
  
LIST(APPEND PV_DOCUMENTATION_FILES 
  ${PV_DOCUMENTATION_HTML_FILES} 
  ${PV_DOCUMENTATION_PNG_FILES}
  ${PV_DOCUMENTATION_JPG_FILES})

foreach (file ${PV_DOCUMENTATION_FILES})
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/${file}"
    "${CMAKE_CURRENT_BINARY_DIR}/${file}"
    COPYONLY)
endforeach (file)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/ParaViewDoc.css"
  "${CMAKE_CURRENT_BINARY_DIR}/ParaViewDoc.css"
  COPYONLY)
    
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/index.html.in"
    "${CMAKE_CURRENT_BINARY_DIR}/index.html"
    @ONLY)
    
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/about.html.in"
    "${CMAKE_CURRENT_BINARY_DIR}/about.html"
    @ONLY)

OPTION(PARAVIEW_GENERATE_PROXY_DOCUMENTATION
  "Turn on to ensure that the documentation for filters/sources/readers and writers is built."
  OFF)

SET (qhp_dependencies)

IF (PARAVIEW_GENERATE_PROXY_DOCUMENTATION)
  # This dependency ensures that whenever the html files are regenerated, the
  # qch file is regenerated as well.
  SET (qhp_dependencies ${qhp_dependencies}
        ${HTMLDocumentation_FILES})
ENDIF(PARAVIEW_GENERATE_PROXY_DOCUMENTATION)

ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/paraview.qhp
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/paraview.qhp.in
          ${CMAKE_CURRENT_SOURCE_DIR}/GenerateQHP.cmake
          ${qhp_dependencies}
  COMMAND ${CMAKE_COMMAND}
    ARGS
      -D DOCUMENTATION_DIR:PATH=${CMAKE_CURRENT_BINARY_DIR}
      -D INPUT:PATH=${CMAKE_CURRENT_SOURCE_DIR}/paraview.qhp.in
      -D OUTPUT:PATH=${CMAKE_CURRENT_BINARY_DIR}/paraview.qhp
      -P ${CMAKE_CURRENT_SOURCE_DIR}/GenerateQHP.cmake
  COMMENT "Generating paraview.qhp"
)

ADD_CUSTOM_TARGET(GenerateParaViewQHP
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/paraview.qhp)

IF (PARAVIEW_GENERATE_PROXY_DOCUMENTATION)
  # This dependency ensures that the ParaViewFiles.html and other files are
  # generated before the qch file is attempted to be generated.
  ADD_DEPENDENCIES(GenerateParaViewQHP
    HTMLDocumentation)
ENDIF (PARAVIEW_GENERATE_PROXY_DOCUMENTATION)

FIND_PROGRAM(QT_HELP_GENERATOR
  qhelpgenerator
  PATHS "${QT_BINARY_DIR}"
  DOC "qhelpgenerator used to compile Qt help project files")

MARK_AS_ADVANCED(PARAVIEW_GENERATE_PROXY_DOCUMENTATION)

ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/paraview.qch
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/paraview.qhp
  COMMAND ${QT_HELP_GENERATOR}
  ARGS ${CMAKE_CURRENT_BINARY_DIR}/paraview.qhp
      -o ${CMAKE_CURRENT_BINARY_DIR}/paraview.qch
  COMMENT "Compiling Qt help project paraview.qhp"
)

ADD_CUSTOM_TARGET(ParaViewOnlineHelp
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/paraview.qch)
ADD_DEPENDENCIES(ParaViewOnlineHelp
  GenerateParaViewQHP)

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
