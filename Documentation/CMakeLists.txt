configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pqClient.adp"
  "${ParaView_BINARY_DIR}/pqClient.adp"
  COPYONLY)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/index.html"
  "${CMAKE_CURRENT_BINARY_DIR}/index.html"
  COPYONLY)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/ParaViewLogo.png"
  "${CMAKE_CURRENT_BINARY_DIR}/ParaViewLogo.png"
  COPYONLY)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/ParaViewDoc.css"
  "${CMAKE_CURRENT_BINARY_DIR}/ParaViewDoc.css"
  COPYONLY)

set(dir "${ParaView_BINARY_DIR}/Documentation/")
install(DIRECTORY ${ParaView_BINARY_DIR}/Documentation/ 
        DESTINATION ${PV_EXE_INSTALL_CM24}/Documentation 
        COMPONENT Runtime
        REGEX ".html")

install(FILES ${ParaView_BINARY_DIR}/pqClient.adp 
  DESTINATION ${PV_EXE_INSTALL_CM24} COMPONENT Runtime )
