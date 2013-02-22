function(copy_example src_dir tgt_dir)
  file(GLOB_RECURSE files
    RELATIVE ${src_dir}
    "${src_dir}/*.*")

  foreach(_file ${files})
    configure_file("${src_dir}/${_file}" "${tgt_dir}/${_file}"
      COPYONLY)
  endforeach()
endfunction()

function(copy_js tgt_dir)
  file(GLOB_RECURSE files
    RELATIVE ${ParaView_DIR}/www/js
    "${ParaView_DIR}/www/js/*.js")

  foreach(_file ${files})
    file(COPY ${ParaView_DIR}/www/js/${_file}
      DESTINATION ${tgt_dir})
  endforeach()
endfunction()

copy_example("${CMAKE_CURRENT_SOURCE_DIR}/can"
  "${CMAKE_CURRENT_BINARY_DIR}/can")
copy_js("${CMAKE_CURRENT_BINARY_DIR}/can/js")

copy_example("${CMAKE_CURRENT_SOURCE_DIR}/cone"
  "${CMAKE_CURRENT_BINARY_DIR}/cone")
copy_js("${CMAKE_CURRENT_BINARY_DIR}/cone/js")

copy_example("${CMAKE_CURRENT_SOURCE_DIR}/live_articles"
  "${CMAKE_CURRENT_BINARY_DIR}/live_articles")
copy_js("${CMAKE_CURRENT_BINARY_DIR}/live_articles/js")