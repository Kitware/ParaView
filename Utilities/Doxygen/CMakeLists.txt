#
# Build the documentation
#
INCLUDE (Documentation)

IF (BUILD_DOCUMENTATION)
  SET(BUILD_DOXYGEN ON)
  SET(DOCUMENTATION_DOWNLOAD_VTK_TAGFILE ON)
  SET(DOCUMENTATION_HTML_TARZ ON)
  SET(DOXYGEN_PROJECT_NAME "ParaQ")
  SET(DOXYGEN_SOURCE_DIR "${ParaQ_SOURCE_DIR}/VTK/Utilities/Doxygen")
  SET(DOXYGEN_PROJECT_SOURCE_DIR "${ParaQ_SOURCE_DIR}")
  SET(VTK_DOXYGEN_HOME "${ParaQ_SOURCE_DIR}/VTK/Utilities/Doxygen")
  SET(DOXYGEN_CVSWEB_CHECKOUT "http://public.kitware.com/cgi-bin/cvsweb.cgi/~checkout~/ParaQ/")
  SET(DOXYGEN_CVSWEB_CHECKOUT_SUFFIX "?cvsroot=ParaQ")
  SET(DOXYGEN_SOURCE_LOCATIONS_DIR "${ParaQ_SOURCE_DIR}")
  SET(DOXYGEN_SOURCE_LOCATIONS
       "\"${ParaQ_SOURCE_DIR}/Utilities/Xdmf/vtk\" \\
        \"${ParaQ_SOURCE_DIR}/Utilities/pqPython\" \\
        \"${ParaQ_SOURCE_DIR}/GUI/Widgets\" \\
        \"${ParaQ_SOURCE_DIR}/GUI/Widgets/Chart\" \\
        \"${ParaQ_SOURCE_DIR}/GUI/Widgets/Testing\" \\
        \"${ParaQ_SOURCE_DIR}/GUI/Client/Client\" \\
        \"${ParaQ_SOURCE_DIR}/GUI/Client/Widgets\" \\
        \"${ParaQ_SOURCE_DIR}/GUI/Client/Components\" \\
        \"${ParaQ_SOURCE_DIR}/GUI/Client/Python\" \\
        \"${ParaQ_SOURCE_DIR}/Common/KWCommon\" \\
        \"${ParaQ_SOURCE_DIR}/Servers/Common\" \\
        \"${ParaQ_SOURCE_DIR}/Servers/Filters\" \\
        \"${ParaQ_SOURCE_DIR}/Servers/ServerManager\"")
  SET(DOXYGEN_BINARY_LOCATIONS_DIR "${ParaQ_BINARY_DIR}")
  SET(DOXYGEN_BINARY_LOCATIONS
       "")
  SET(DOXYGEN_PARSED_INPUT_DIRS
      " \"${CMAKE_CURRENT_BINARY_DIR}/dox/Utilities/Xdmf/vtk\" \\
        \"${ParaQ_SOURCE_DIR}/Utilities/pqPython\" \\
        \"${ParaQ_SOURCE_DIR}/GUI/Widgets\" \\
        \"${ParaQ_SOURCE_DIR}/GUI/Widgets/Chart\" \\
        \"${ParaQ_SOURCE_DIR}/GUI/Widgets/Testing\" \\
        \"${ParaQ_SOURCE_DIR}/GUI/Client/Client\" \\
        \"${ParaQ_SOURCE_DIR}/GUI/Client/Widgets\" \\
        \"${ParaQ_SOURCE_DIR}/GUI/Client/Components\" \\
        \"${ParaQ_SOURCE_DIR}/GUI/Client/Python\" \\
        \"${CMAKE_CURRENT_BINARY_DIR}/dox/GUI/Client/Widgets\" \\
        \"${CMAKE_CURRENT_BINARY_DIR}/dox/Common/KWCommon\" \\
        \"${CMAKE_CURRENT_BINARY_DIR}/dox/Servers/Common\" \\
        \"${CMAKE_CURRENT_BINARY_DIR}/dox/Servers/Filters\" \\
        \"${CMAKE_CURRENT_BINARY_DIR}/dox/Servers/ServerManager\"")

  SET(DOXYGEN_IGNORE_PREFIX "vtkPV vtkSM vtk pq")

  #
  # Configure the script and the doxyfile, then add target
  #
  IF(NOT DOT_PATH)
    GET_FILENAME_COMPONENT(DOT_PATH ${DOT} PATH)
  ENDIF(NOT DOT_PATH)

  CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.in
    ${CMAKE_CURRENT_BINARY_DIR}/doxyfile @ONLY IMMEDIATE)

  CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/doc_makeall.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/doc_makeall.sh @ONLY IMMEDIATE)

  ADD_CUSTOM_TARGET(${DOXYGEN_PROJECT_NAME}DoxygenDoc
    ${BASH}
    ${CMAKE_CURRENT_BINARY_DIR}/doc_makeall.sh)

ENDIF (BUILD_DOCUMENTATION)
