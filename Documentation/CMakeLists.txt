file(MAKE_DIRECTORY ${PARAVIEW_GENERATED_DOCUMENTATION_OUTPUT_DIRECTORY})

#---------------------------------------------------------------------------------
# add a txt file to indicate which source version the documentation was
# generated for.
set(CMAKE_CONFIGURABLE_FILE_CONTENT "${PARAVIEW_VERSION_FULL}")
configure_file("${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in"
               "${PARAVIEW_GENERATED_DOCUMENTATION_OUTPUT_DIRECTORY}/version.txt")
unset(CMAKE_CONFIGURABLE_FILE_CONTENT)

#---------------------------------------------------------------------------------
# add a custom command to package the documentations into a tarball that's
# easy to move around.
add_custom_target(ParaViewDoc-TGZ
  COMMAND
    ${CMAKE_COMMAND} -E tar cfz
    ${CMAKE_CURRENT_BINARY_DIR}/ParaViewGeneratedDocs.tar.gz
    ${PARAVIEW_GENERATED_DOCUMENTATION_OUTPUT_DIRECTORY}
  WORKING_DIRECTORY ${PARAVIEW_GENERATED_DOCUMENTATION_OUTPUT_DIRECTORY}/..
  COMMENT "Generate ParaViewGeneratedDocs.tar.gz")

add_dependencies(ParaViewDoc-TGZ ParaViewDoxygenDoc)
if (PARAVIEW_ENABLE_PYTHON)
  add_dependencies(ParaViewDoc-TGZ ParaViewPythonDoc)
endif ()
