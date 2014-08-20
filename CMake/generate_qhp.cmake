# Used by build_help_project CMake function to generate the qhp file.
# The WORKING_DIRECTORY for this script must be the same as the location for the
# qhp file otherwise the toc won't be generated correctly.
# Input environment:
# input_file :-
# output_file :-
# file_patterns :-
# namespace :-
# folder :-
# name :-

# extracts title from the html page to generate a user-friendly index.
function (extract_title name filename)
  file (READ ${filename} contents)
  string (REGEX MATCH "<title>(.*)</title>" tmp "${contents}")
  if (CMAKE_MATCH_1)
    set (${name} "${CMAKE_MATCH_1}" PARENT_SCOPE)
  else ()
    get_filename_component(filename_name "${filename}" NAME)
    set (${name} "${filename_name}" PARENT_SCOPE)
  endif()
endfunction()

if (NOT output_file OR NOT file_patterns OR NOT namespace OR NOT folder OR NOT name)
  message(FATAL_ERROR "Missing one of the required arguments!!")
endif ()

string (REPLACE "+" ";" file_patterns "${file_patterns}")

get_filename_component(working_dir "${output_file}" PATH)

# We generate a toc using the files present.
file (GLOB matching_files RELATIVE "${CMAKE_CURRENT_BINARY_DIR}" ${file_patterns} )
set (toc)
set (index_page)
foreach (filename ${matching_files})
  string(REGEX MATCH "^(.*)\\.html$" _tmp "${filename}")
  set (name_we ${CMAKE_MATCH_1})
  if (name_we)
    extract_title(title "${filename}")
    set (toc "${toc}    <section title=\"${title}\" ref=\"${filename}\" />\n")
    
    string(TOLOWER "${filename}" lowercase_filename)
    if (lowercase_filename MATCHES "index.html$")
      set (index_page "${filename}")
    endif ()
  endif()
endforeach()

if (NOT index_page AND matching_files)
  # no index.html file located. Just point to the first html, if any.
  list(GET matching_files 0 index_page)
endif()

set (toc
  "<toc> <section title=\"${name}\" ref=\"${index_page}\" >\n ${toc} </section> </toc>")

set (files)
foreach(filename ${file_patterns})
  set (files "${files}<file>${filename}</file>\n")
endforeach()

configure_file(
  "${input_file}"
  "${output_file}"
  @ONLY)
