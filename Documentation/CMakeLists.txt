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

OPTION(PARAVIEW_GENERATE_PROXY_DOCUMENTATION
  "Turn on to ensure that the documentation for filters/sources/readers and writers is built."
  OFF)

FIND_PROGRAM(QT_HELP_GENERATOR
  qhelpgenerator
  PATHS "${QT_BINARY_DIR}"
  DOC "qhelpgenerator used to compile Qt help project files")
MARK_AS_ADVANCED(PARAVIEW_GENERATE_PROXY_DOCUMENTATION)

set (extra_dependencies)
IF (PARAVIEW_GENERATE_PROXY_DOCUMENTATION)
  # This dependency ensures that the ParaViewFiles.html and other files are
  # generated before the qch file is attempted to be generated.
  set(extra_dependencies HTMLDocumentation)
ENDIF (PARAVIEW_GENERATE_PROXY_DOCUMENTATION)

ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/paraview.qch
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/paraview.qhp
          ${extra_dependencies}
  COMMAND ${QT_HELP_GENERATOR}
  ARGS ${CMAKE_CURRENT_BINARY_DIR}/paraview.qhp
      -o ${CMAKE_CURRENT_BINARY_DIR}/paraview.qch
  COMMENT "Compiling Qt help project paraview.qhp"
)

ADD_CUSTOM_TARGET(ParaViewOnlineHelp
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
