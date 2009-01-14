#
# Build the documentation
#
INCLUDE (Documentation)

IF (BUILD_DOCUMENTATION)
  SET (INCLUDE_QT_DOCUMENTATION OFF CACHE BOOL "Specify if Qt documentation should be linked to.")
  MARK_AS_ADVANCED(INCLUDE_QT_DOCUMENTATION)
  IF (INCLUDE_QT_DOCUMENTATION)
    SET(QT_DOC_DIR CACHE FILEPATH "Path to the directory containing Qt html documentation.")
    FIND_PROGRAM (DOXYTAG
      doxytag
      "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\doxygen_is1;Inno Setup: App Path]/bin"
      /Applications/Doxygen.app/Contents/Resources
      /Applications/Doxygen.app/Contents/MacOS
      )
    MARK_AS_ADVANCED(DOXYTAG QT_DOC_DIR)
    SET (QT_TAG_FILE "${CMAKE_CURRENT_BINARY_DIR}/Qt.tag")
  ENDIF (INCLUDE_QT_DOCUMENTATION)
  SET(BUILD_DOXYGEN ON)
  SET(DOCUMENTATION_DOWNLOAD_VTK_TAGFILE ON)
  SET(DOCUMENTATION_HTML_TARZ ON)
  SET(DOXYGEN_PROJECT_NAME "ParaView")
  SET(DOXYGEN_SOURCE_DIR "${ParaView_SOURCE_DIR}/VTK/Utilities/Doxygen")
  SET(DOXYGEN_PROJECT_SOURCE_DIR "${ParaView_SOURCE_DIR}")
  SET(VTK_DOXYGEN_HOME "${ParaView_SOURCE_DIR}/VTK/Utilities/Doxygen")
  SET(DOXYGEN_CVSWEB_CHECKOUT "http://public.kitware.com/cgi-bin/cvsweb.cgi/~checkout~/ParaView3/")
  SET(DOXYGEN_CVSWEB_CHECKOUT_SUFFIX "?cvsroot=ParaView3")
  SET(DOXYGEN_SOURCE_LOCATIONS_DIR "${ParaView_SOURCE_DIR}")
  SET(DOXYGEN_SOURCE_LOCATIONS
       "\"${ParaView_SOURCE_DIR}/Utilities/Xdmf2/vtk\" \\
        \"${ParaView_SOURCE_DIR}/Utilities/VTKPythonWrapping/Executable\" \\
        \"${ParaView_SOURCE_DIR}/Utilities/VTKClientServer\" \\
        \"${ParaView_SOURCE_DIR}/Qt/Chart\" \\
        \"${ParaView_SOURCE_DIR}/Qt/Components\" \\
        \"${ParaView_SOURCE_DIR}/Qt/Core\" \\
        \"${ParaView_SOURCE_DIR}/Qt/Python\" \\
        \"${ParaView_SOURCE_DIR}/Qt/Widgets\" \\
        \"${ParaView_SOURCE_DIR}/Qt/Testing\" \\
        \"${ParaView_SOURCE_DIR}/Applications/Client\" \\
        \"${ParaView_SOURCE_DIR}/Applications/OverView\" \\
        \"${ParaView_SOURCE_DIR}/Applications/OverView/Core\" \\
        \"${ParaView_SOURCE_DIR}/Common/KWCommon\" \\
        \"${ParaView_SOURCE_DIR}/Servers/Common\" \\
        \"${ParaView_SOURCE_DIR}/Servers/Filters\" \\
        \"${ParaView_SOURCE_DIR}/Servers/ServerManager\"")
  SET(DOXYGEN_BINARY_LOCATIONS_DIR "${ParaView_BINARY_DIR}")
  SET(DOXYGEN_BINARY_LOCATIONS
       "")
  SET(DOXYGEN_PARSED_INPUT_DIRS
      " \"${CMAKE_CURRENT_BINARY_DIR}/dox/Utilities/Xdmf2/vtk\" \\
        \"${CMAKE_CURRENT_BINARY_DIR}/dox/Utilities/VTKPythonWrapping/Executable\" \\
        \"${ParaView_SOURCE_DIR}/Utilities/VTKClientServer\" \\
        \"${ParaView_SOURCE_DIR}/Qt/Chart\" \\
        \"${ParaView_SOURCE_DIR}/Qt/Components\" \\
        \"${ParaView_SOURCE_DIR}/Qt/Core\" \\
        \"${ParaView_SOURCE_DIR}/Qt/Python\" \\
        \"${ParaView_SOURCE_DIR}/Qt/Widgets\" \\
        \"${ParaView_SOURCE_DIR}/Qt/Testing\" \\
        \"${ParaView_SOURCE_DIR}/Applications/Client\" \\
        \"${ParaView_SOURCE_DIR}/Applications/DobranoViz\" \\
        \"${CMAKE_CURRENT_BINARY_DIR}/dox/Qt/Core\" \\
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

  SET(TAGFILES "\"${CMAKE_CURRENT_BINARY_DIR}/vtkNightlyDoc.tag=http://www.vtk.org/doc/nightly/html\"")
  IF(INCLUDE_QT_DOCUMENTATION)
    SET (TAGFILES "${TAGFILES} \"${QT_TAG_FILE}=http://doc.trolltech.com/4.2\"")
  ENDIF(INCLUDE_QT_DOCUMENTATION)

  CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.in
    ${CMAKE_CURRENT_BINARY_DIR}/doxyfile @ONLY IMMEDIATE)

  CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/doc_makeall.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/doc_makeall.sh @ONLY IMMEDIATE)

  ADD_CUSTOM_TARGET(${DOXYGEN_PROJECT_NAME}DoxygenDoc
    ${BASH}
    ${CMAKE_CURRENT_BINARY_DIR}/doc_makeall.sh)

  IF(INCLUDE_QT_DOCUMENTATION)
    ADD_CUSTOM_TARGET(QtDoxygenTags
      COMMAND ${DOXYTAG} -t ${QT_TAG_FILE} ${QT_DOC_DIR})
    ADD_DEPENDENCIES(${DOXYGEN_PROJECT_NAME}DoxygenDoc
      QtDoxygenTags)
  ENDIF(INCLUDE_QT_DOCUMENTATION)

ENDIF (BUILD_DOCUMENTATION)
