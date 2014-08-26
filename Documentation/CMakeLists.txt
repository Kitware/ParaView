add_custom_target(ParaViewWelcomeDoc
  DEPENDS index-doc.html
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${ParaView_SOURCE_DIR}/Documentation/img" "${PARAVIEW_WWW_DIR}/img"
  COMMAND ${CMAKE_COMMAND} -E copy "${ParaView_SOURCE_DIR}/Documentation/index-doc.html" ${PARAVIEW_WWW_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy "${ParaView_SOURCE_DIR}/Documentation/kitware.css" ${PARAVIEW_WWW_DIR}
)

add_dependencies(ParaViewDoc ParaViewWelcomeDoc)
