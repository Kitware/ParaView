# This file is used by generate_htmls_from_xmls function in ParaViewMacros.cmake
# to split a file consisting of multiple htmls into multiple files.
cmake_minimum_required(VERSION 2.8)

file(READ "${input_file}" multiple_htmls)

# if the contents of input_file contains ';', then CMake gets confused.
# So replace all semicolons with a placeholder.
string(REPLACE ";" "\\semicolon" multiple_htmls "${multiple_htmls}")

# Convert the single string into a list split at </html> markers.
string(REPLACE "</html>" "</html>;" multiple_htmls_as_list "${multiple_htmls}")

# Generate output HTML for each <html>..</html> chunk in the input.
foreach (single_html ${multiple_htmls_as_list})
  set (proxy_name)
  set (group_name)

  string(REGEX MATCH "<meta[^>]*name=\"proxy_name\".*>([a-zA-Z0-9_-]+):([a-zA-Z0-9_-]+)</meta>"
      tmp "${single_html}")
  if (CMAKE_MATCH_2)
    set(proxy_name ${CMAKE_MATCH_2})
  endif()
  if (CMAKE_MATCH_1)
    set(group_name ${CMAKE_MATCH_1})
  endif()

  if (group_name)
    # process formatting strings.
    string (REPLACE "\\semicolon" ";" single_html "${single_html}")
    string (REGEX REPLACE "\\\\bold{([^}]+)}" "<b>\\1</b>" single_html "${single_html}")
    string (REGEX REPLACE "\\\\emph{([^}]+)}" "<i>\\1</i>" single_html "${single_html}")
    file (WRITE "${output_dir}/${group_name}.${proxy_name}.html" "${single_html}")
  endif()
endforeach()

# write the output file
file(WRITE "${output_file}" "done")
