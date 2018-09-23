# add a custom command to package the documentations into a tar ball thats
# easy  to move around.
file(MAKE_DIRECTORY ${PARAVIEW_GENERATED_DOCUMENTATION_OUTPUT_DIRECTORY})

add_custom_target(ParaViewDoc-TGZ
  COMMAND
    ${CMAKE_COMMAND} -E tar cfz
    ${CMAKE_CURRENT_BINARY_DIR}/ParaViewGeneratedDocs.tar.gz
    ${PARAVIEW_GENERATED_DOCUMENTATION_OUTPUT_DIRECTORY}
  WORKING_DIRECTORY ${PARAVIEW_GENERATED_DOCUMENTATION_OUTPUT_DIRECTORY}/..
  COMMENT "Generate ParaViewGeneratedDocs.tar.gz")

add_dependencies(ParaViewDoc-TGZ ParaViewDoc)
