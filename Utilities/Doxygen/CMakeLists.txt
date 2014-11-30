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

# Build up a list of all module source directories. Note that this should be
# all source directories and so does not use the normal variables.
unset(PV_MODULE_DIRS_DOXYGEN)
unset(PV_MODULE_DIRS_DOXYGEN_ESCAPED)
file(GLOB_RECURSE src RELATIVE "${ParaView_SOURCE_DIR}"
    "${ParaView_SOURCE_DIR}/module.cmake")
foreach(module ${src})
  get_filename_component(module_BASE ${module} PATH)
  if("${module_BASE}" MATCHES "^Utilities" OR ${module_BASE} MATCHES "^Third" OR
      "${module_BASE}" MATCHES "^VTK" OR "${module_BASE}" MATCHES "^Plugins")
  else()
    set(PV_MODULE_DIRS_DOXYGEN
      "${PV_MODULE_DIRS_DOXYGEN}  \"${ParaView_SOURCE_DIR}/${module_BASE}\"\n")
    set(PV_MODULE_DIRS_DOXYGEN_ESCAPED
      "${PV_MODULE_DIRS_DOXYGEN_ESCAPED} \"${ParaView_SOURCE_DIR}/${module_BASE}\"\\\n")
  endif()
endforeach()

set(TAGFILES "\"${CMAKE_CURRENT_BINARY_DIR}/vtkNightlyDoc.tag=http://www.vtk.org/doc/nightly/html\"")
set (QT_TAG_FILE "${QT_DOC_DIR}/html/qt.tags")
if (EXISTS "${QT_TAG_FILE}")
  SET (TAGFILES "${TAGFILES} \"${QT_TAG_FILE}=http://qt-project.org/doc/qt-4.8/\"")
endif()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/pages/index.md.in
  ${CMAKE_CURRENT_BINARY_DIR}/pages/index.md)

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

add_custom_target(ParaViewDoxygenDoc-copy
  DEPENDS ParaViewDoxygenDoc
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${ParaView_BINARY_DIR}/Utilities/Doxygen/doc/html "${PARAVIEW_WWW_DIR}/cxx-doc"
)

add_dependencies(ParaViewDoc ParaViewDoxygenDoc-copy)
