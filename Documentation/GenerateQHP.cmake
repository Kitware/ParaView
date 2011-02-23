# Script used to generate paraview.qhp file.

file(GLOB files RELATIVE "${DOCUMENTATION_DIR}" 
  "${DOCUMENTATION_DIR}/*.*"
  "${DOCUMENTATION_DIR}/Book/*.*")

SET (DOCUMENTATION_FILES)

foreach (file ${files})
  set (DOCUMENTATION_FILES 
    "${DOCUMENTATION_FILES}\n          <file>${file}</file>")
endforeach (file)

configure_file(${INPUT} ${OUTPUT})
