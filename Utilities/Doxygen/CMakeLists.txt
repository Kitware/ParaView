#
# Build the documentation
#

INCLUDE (${CMAKE_ROOT}/Modules/Documentation.cmake)

IF (BUILD_DOCUMENTATION)
  SET(BUILD_DOXYGEN ON)
  SET(DOCUMENTATION_DOWNLOAD_VTK_TAGFILE ON)
  SET(DOCUMENTATION_HTML_TARZ ON)
  SET(DOXYGEN_PROJECT_NAME "ParaView")
  SET(DOXYGEN_SOURCE_DIR "${ParaView_SOURCE_DIR}/VTK/Utilities/Doxygen")
  SET(DOXYGEN_PROJECT_SOURCE_DIR "${ParaView_SOURCE_DIR}")
  SET(VTK_DOXYGEN_HOME "${ParaView_SOURCE_DIR}/VTK/Utilities/Doxygen")
  SET(DOXYGEN_CVSWEB_CHECKOUT "http://public.kitware.com/cgi-bin/cvsweb.cgi/~checkout~/ParaView/")
  SET(DOXYGEN_CVSWEB_CHECKOUT_SUFFIX "?cvsroot=ParaView")
  SET(DOXYGEN_SOURCE_LOCATIONS_DIR "${ParaView_SOURCE_DIR}")
  SET(DOXYGEN_SOURCE_LOCATIONS
       "\"${ParaView_SOURCE_DIR}/Utilities/Xdmf/vtk\" \\
        \"${ParaView_SOURCE_DIR}/Common/KWCommon\" \\
        \"${ParaView_SOURCE_DIR}/Servers/Common\" \\
        \"${ParaView_SOURCE_DIR}/Servers/Filters\" \\
        \"${ParaView_SOURCE_DIR}/Servers/ServerManager\" \\
        \"${ParaView_SOURCE_DIR}/GUI/Widgets\" \\
        \"${ParaView_SOURCE_DIR}/GUI/Client\"")
  SET(DOXYGEN_BINARY_LOCATIONS_DIR "${ParaView_BINARY_DIR}")
  SET(DOXYGEN_BINARY_LOCATIONS
     "\"@ParaView_BINARY_DIR@/GUI/Widgets/Templates\"")
  SET(DOXYGEN_PARSED_INPUT_DIRS
" \"${CMAKE_CURRENT_BINARY_DIR}/dox/Utilities/Xdmf/vtk\" \\
 \"${CMAKE_CURRENT_BINARY_DIR}/dox/Common/KWCommon\" \\
 \"${CMAKE_CURRENT_BINARY_DIR}/dox/Servers/Common\" \\
 \"${CMAKE_CURRENT_BINARY_DIR}/dox/Servers/Filters\" \\
 \"${CMAKE_CURRENT_BINARY_DIR}/dox/Servers/ServerManager\" \\
 \"${CMAKE_CURRENT_BINARY_DIR}/dox/GUI/Widgets\" \\
 \"${CMAKE_CURRENT_BINARY_DIR}/dox/GUI/Widgets/Templates\" \\
 \"${CMAKE_CURRENT_BINARY_DIR}/dox/GUI/Client\"")

  SET(DOXYGEN_IGNORE_PREFIX "vtkKW vtkPV vtkSM vtk")

  SET(RM "NOTFOUND")

  #
  # Configure the script and the doxyfile, then add target
  #
  IF(NOT DOT_PATH)
    GET_FILENAME_COMPONENT(DOT_PATH ${DOT} PATH)
  ENDIF(NOT DOT_PATH)

  CONFIGURE_FILE(
    ${ParaView_SOURCE_DIR}/Utilities/Doxygen/doxyfile.in
    ${ParaView_BINARY_DIR}/Utilities/Doxygen/doxyfile @ONLY IMMEDIATE)

  CONFIGURE_FILE(
    ${ParaView_SOURCE_DIR}/Utilities/Doxygen/doc_makeall.sh.in
    ${ParaView_BINARY_DIR}/Utilities/Doxygen/doc_makeall.sh @ONLY IMMEDIATE)

  ADD_CUSTOM_TARGET(ParaViewDoxygenDoc
    ${BASH}
    ${ParaView_BINARY_DIR}/Utilities/Doxygen/doc_makeall.sh)

ENDIF (BUILD_DOCUMENTATION)
