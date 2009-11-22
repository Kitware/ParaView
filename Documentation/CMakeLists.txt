# Copy all online help files from current directory over to the binary directory.
# These do not include any server manager documentation htmls.
file(GLOB PV_DOCUMENTATION_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/[^.]*.*)

foreach (file ${PV_DOCUMENTATION_FILES})
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/${file}"
    "${CMAKE_CURRENT_BINARY_DIR}/${file}"
    COPYONLY)
endforeach (file)


ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/paraview.qhp
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/paraview.qhp.in
          ${CMAKE_CURRENT_SOURCE_DIR}/GenerateQHP.cmake
  COMMAND ${CMAKE_COMMAND}
    ARGS
      -D DOCUMENTATION_DIR:PATH=${CMAKE_CURRENT_BINARY_DIR}
      -D INPUT:PATH=${CMAKE_CURRENT_SOURCE_DIR}/paraview.qhp.in
      -D OUTPUT:PATH=${CMAKE_CURRENT_BINARY_DIR}/paraview.qhp
      -P ${CMAKE_CURRENT_SOURCE_DIR}/GenerateQHP.cmake
  COMMENT "Generating paraview.qhp"
)

FIND_PROGRAM(QT_HELP_GENERATOR
  qhelpgenerator
  PATHS "${QT_BINARY_DIR}"
  DOC "qhelpgenerator used to compile Qt help project files")

ADD_CUSTOM_TARGET(ParaViewOnlineHelp
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/paraview.qhp
  COMMAND ${QT_HELP_GENERATOR}
  ARGS ${CMAKE_CURRENT_BINARY_DIR}/paraview.qhp
      -o ${CMAKE_CURRENT_BINARY_DIR}/paraview.qch
  COMMENT "Compiling Qt help project paraview.qhp"
)

ADD_DEPENDENCIES(ParaViewOnlineHelp
  HTMLDocumentation
  )
