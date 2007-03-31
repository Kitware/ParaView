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

set(dir "${ParaView_BINARY_DIR}/Documentation/")
file(GLOB list_of_files "${dir}/*")
foreach(file ${list_of_files})
  if (${file} MATCHES .html OR ${file} MATCHES .png)
    install(FILES ${file}
      DESTINATION ${PV_EXE_INSTALL_CM24}/Documentation COMPONENT Runtime )
    endif (${file} MATCHES .html OR ${file} MATCHES .png)
endforeach(file)

install(FILES ${ParaView_BINARY_DIR}/pqClient.adp 
  DESTINATION ${PV_EXE_INSTALL_CM24} COMPONENT Runtime )
