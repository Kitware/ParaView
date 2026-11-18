#
# Build the documentation
#
find_package(Doxygen REQUIRED)
find_package(Perl REQUIRED)
find_package(HTMLHelp)

if (DOXYGEN_VERSION VERSION_LESS "1.8.0")
  message(FATAL_ERROR
    "Doxygen version 1.8 or greater is required (for Markdown support)")
endif()

find_program(GUNZIPCOMMAND gunzip DOC "Path to gunzip executable")
mark_as_advanced(GUNZIPCOMMAND)

set(DOXYGEN_SHORT_NAMES NO)
#
# Configure the script and the doxyfile, then add target
#
set(HAVE_DOT_YESNO NO)
if(DOT)
  set(HAVE_DOT_YESNO YES)
  if(NOT DOT_PATH)
    get_filename_component(DOT_PATH ${DOT} PATH)
  endif()
endif()

set(TAGFILES "\"${CMAKE_CURRENT_BINARY_DIR}/vtkNightlyDoc.tag=http://www.vtk.org/doc/nightly/html\"")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/pages/index.md.in
  ${CMAKE_CURRENT_BINARY_DIR}/pages/index.md)

# this var tells doxyfile where to generate the html pages.
# in doxyfile we place all html pages relative to this dir under "cxx"
# (see `HTML_OUTPUT`) var in doxyfile.in
set(DOCUMENTATION_OUTPUT_DIRECTORY
  "${PARAVIEW_GENERATED_DOCUMENTATION_OUTPUT_DIRECTORY}")

configure_file(
  ${ParaView_SOURCE_DIR}/Utilities/Doxygen/doxyfile.in
  ${ParaView_BINARY_DIR}/Utilities/Doxygen/doxyfile)

configure_file(
  ${ParaView_SOURCE_DIR}/Utilities/Doxygen/doc_makeall.cmake.in
  ${ParaView_BINARY_DIR}/Utilities/Doxygen/doc_makeall.cmake
  @ONLY)

add_custom_target(ParaViewDoxygenDoc
  ${CMAKE_COMMAND}
  -P ${ParaView_BINARY_DIR}/Utilities/Doxygen/doc_makeall.cmake
  DEPENDS ${ParaView_BINARY_DIR}/Utilities/Doxygen/doc_makeall.cmake)
