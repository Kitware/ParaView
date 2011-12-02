# Used by build_help_project CMake function to generate the qhp file.
# The WORKING_DIRECTORY for this script must be the same as the location for the
# qhp file otherwise the toc won't be generated correctly.
# Input environment:
# output_file :-
# file_patterns :-
# namespace :-
# folder :-
# name :-

set (qhp_contents
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<QtHelpProject version=\"1.0\">
    <namespace>@namespace@</namespace>
    <virtualFolder>@folder@</virtualFolder>
    <filterSection>
        @toc@
        <keywords>
          <!-- how to handle keywords? -->
        </keywords>
        <files>
          @files@
        </files>
    </filterSection>
</QtHelpProject>")

if (NOT output_file OR NOT file_patterns OR NOT namespace OR NOT folder OR NOT name)
  message(FATAL_ERROR "Missing one of the required arguments!!")
endif ()

string (REPLACE "+" ";" file_patterns "${file_patterns}")

get_filename_component(working_dir "${output_file}" PATH)

# We generate a toc using the files present.
file (GLOB matching_files RELATIVE "${CMAKE_CURRENT_BINARY_DIR}" ${file_patterns} )
set (toc)
foreach (filename ${matching_files})
  string (REGEX MATCH "^(.*)\\.html$" _tmp "${filename}")
  set (name_we ${CMAKE_MATCH_1})
  if (name_we)
    get_filename_component(filename_name "${filename}" NAME)
    set (toc 
    "${toc}    <section title=\"${filename_name}\" ref=\"${filename}\" />\n")
  endif()
endforeach()
set (toc
"<toc>
  <section title=\"${name}\" ref=\"index.html\" >\n
${toc}
  </section>
</toc>")

set (files)
foreach(filename ${file_patterns})
  set (files "${files}<file>${filename}</file>\n")
endforeach()

string(CONFIGURE "${qhp_contents}" text @ONLY)
file (WRITE "${output_file}" "${text}")
