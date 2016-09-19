# This file is used by generate_htmls_from_xmls function in ParaViewMacros.cmake
# to split a file consisting of multiple htmls into multiple files.
cmake_minimum_required(VERSION 3.3)

# INPUT VARIABLES:
# xmlpatterns       :- xmlpatterns executable.
# xml_to_xml_xsl    :- xsl file to convert SM xml to internal xml Model.
# generate_category_rw_xsl
#                   :- xsl file to generate a categoryindex for Readers and Writers
# xml_to_html_xsl   :- xsl file to conevrt the internal xml to html.
# xml_to_wiki_xsl   :- xsl file to conevrt the internal xml to wiki.
# input_xmls        :- coded-separator list of SM xml files
# input_gui_xmls    :- coded-separator list of GUI xml files used to generate the
#                        CatergoryIndex.html
# output_dir        :- Directory where all HTMLs are written out.
# output_file       :- File written out on successful completion.
#                      This file is also used to save intermediate results.
#
# see ParaViewMacros.cmake for information about coded-separator lists

if (NOT EXISTS "${xmlpatterns}")
  message(FATAL_ERROR "No xmlpatterns executable was defined!!!")
endif()

# input_xmls is a pseudo-list. Convert it to a real CMake list.
string(REPLACE "_s" ";"  input_xmls "${input_xmls}")
string(REPLACE "_u" "_"  input_xmls "${input_xmls}")
string(REPLACE "_s" ";"  input_gui_xmls "${input_gui_xmls}")
string(REPLACE "_u" "_"  input_gui_xmls "${input_gui_xmls}")

set (xslt_xml)

# Generate intermediate xml using the input_xmls and XSL file.
# It also processes GUI xmls to add catergory-index sections.

foreach(xml ${input_xmls} ${input_gui_xmls})
  get_filename_component(xml_name_we  ${xml} NAME_WE)
  get_filename_component(xml_name  ${xml} NAME)

  set (temp)
  # process each XML using the XSL to generate the html.
  execute_process(
    # create directory first. This is not needed, but if no output htmls are
    # generated because of missing proxy definitions, the the build may die with
    # an error due to missing directory.
    COMMAND ${CMAKE_COMMAND} -E make_directory "${output_dir}"

    # do the XSL translations.
    COMMAND "${xmlpatterns}" "${xml_to_xml_xsl}" "${xml}"
    OUTPUT_VARIABLE  temp
    )
    
  # combine results.
  set (xslt_xml "${xslt_xml}\n${temp}")
endforeach()

# write the combined XML out in a single file.
set (xslt_xml "<xml>\n${xslt_xml}\n</xml>")
file (WRITE "${output_file}" "${xslt_xml}")

# process the temporary xml to generate categoryindex for readers and writers
execute_process(
  COMMAND "${xmlpatterns}"
          "${generate_category_rw_xsl}"
          "${output_file}"
          OUTPUT_VARIABLE temp
  )
file (WRITE "${output_file}" "${temp}")

# process the temporary.xml using the second XSL to generate a combined html
# file.
set (multiple_htmls)
execute_process(
  COMMAND "${xmlpatterns}"
          "${xml_to_html_xsl}"
          "${output_file}"
  OUTPUT_VARIABLE multiple_htmls
  )

# if the contents of input_file contains ';', then CMake gets confused.
# So replace all semicolons with a placeholder.
string(REPLACE ";" "\\semicolon" multiple_htmls "${multiple_htmls}")

# Convert the single string into a list split at </html> markers.
string(REPLACE "</html>" "</html>;" multiple_htmls_as_list "${multiple_htmls}")

# Generate output HTML for each <html>..</html> chunk in the input.
foreach (single_html ${multiple_htmls_as_list})
  string(REGEX MATCH "<meta name=\"filename\" contents=\"([a-zA-Z0-9._-]+)\"" tmp "${single_html}")
  set (filename ${CMAKE_MATCH_1})
  if (filename)
    # revert the semicolon placeholder
    string (REPLACE "\\semicolon" ";" single_html "${single_html}")

    # convert RST formatting strings into HTML
    # bold
    string (REGEX REPLACE "[*][*]([^*]+)[*][*]" "<b>\\1</b>" single_html 
      "${single_html}")
    # italic
    string (REGEX REPLACE "[*]([^*]+)[*]" "<i>\\1</i>" single_html 
      "${single_html}")
    # unordered list
    string (REPLACE "\n\n- " "\n<ul><li>" single_html "${single_html}")
    string (REPLACE "\n- " "\n<li>" single_html "${single_html}")
    string (REGEX REPLACE "<li>(.*)\n\n([^-])" "<li>\\1</ul>\n\\2" single_html 
      "${single_html}")
    # paragraph
    string (REPLACE "\n\n" "\n<p>\n" single_html "${single_html}")
    file (WRITE "${output_dir}/${filename}" "${single_html}")
  endif()
endforeach()

#                     ----------- WIKI -----------
# process the temporary.xml using the thrird XSL to generate a wiki content
set(wiki_sections sources filters writers)
foreach(wiki_section ${wiki_sections})
  message("Processing wiki ${wiki_section}")
  set(GROUP ${wiki_section})
  set(QUERY "not(contains(lower-case($proxy_name),'reader'))")
  set(tmp_wiki_xsl ${CMAKE_CURRENT_BINARY_DIR}/${wiki_section}_xml_to_wiki.xsl)
  set(wiki_file ${output_dir}/${wiki_section}.wiki)
  CONFIGURE_FILE(
        ${xml_to_wiki_xsl}
        ${tmp_wiki_xsl}
        @ONLY IMMEDIATE)
  execute_process(
        COMMAND "${xmlpatterns}"
                "${tmp_wiki_xsl}"
                "${output_file}"
        OUTPUT_VARIABLE wiki_content)
  string (REGEX REPLACE " +" " " wiki_content "${wiki_content}")
  string (REGEX REPLACE "\n " "\n" wiki_content "${wiki_content}")
  file (WRITE "${wiki_file}" "${wiki_content}")
endforeach()

# Handle readers...
message("Processing wiki readers")
set(GROUP sources)
set(QUERY "contains(lower-case($proxy_name),'reader')")
set(tmp_wiki_xsl ${CMAKE_CURRENT_BINARY_DIR}/readers_xml_to_wiki.xsl)
set(wiki_file ${output_dir}/readers.wiki)
CONFIGURE_FILE(
        ${xml_to_wiki_xsl}
        ${tmp_wiki_xsl}
        @ONLY IMMEDIATE)
execute_process(
        COMMAND "${xmlpatterns}"
                "${tmp_wiki_xsl}"
                "${output_file}"
        OUTPUT_VARIABLE wiki_content)
string (REGEX REPLACE " +" " " wiki_content "${wiki_content}")
string (REGEX REPLACE "\n " "\n" wiki_content "${wiki_content}")
file (WRITE "${wiki_file}" "${wiki_content}")
