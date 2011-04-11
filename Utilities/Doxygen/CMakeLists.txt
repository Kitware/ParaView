#
# Build the documentation
#
INCLUDE (Documentation)

IF (BUILD_DOCUMENTATION)
  # Since Qt 4.6, Nokia distributes the qt.tag file. So we directly use it.
  SET (QT_TAG_FILE "${QT_DOC_DIR}/html/qt.tags")

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
        \"${ParaView_SOURCE_DIR}/Qt/Core\" \\
        \"${ParaView_SOURCE_DIR}/Common/KWCommon\" \\
        \"${ParaView_SOURCE_DIR}/ParaViewCore/Common\" \\
        \"${ParaView_SOURCE_DIR}/ParaViewCore/ClientServerCore\" \\
        \"${ParaView_SOURCE_DIR}/ParaViewCore/ServerImplementation\" \\
        \"${ParaView_SOURCE_DIR}/ParaViewCore/ServerManager\" \\
        \"${ParaView_SOURCE_DIR}/ParaViewCore/VTKExtensions\"")
  SET(DOXYGEN_BINARY_LOCATIONS_DIR "${ParaView_BINARY_DIR}")
  SET(DOXYGEN_BINARY_LOCATIONS
       "")
  SET(DOXYGEN_PARSED_INPUT_DIRS
      " \"${CMAKE_CURRENT_BINARY_DIR}/dox/Utilities/Xdmf2/vtk\" \\
        \"${CMAKE_CURRENT_BINARY_DIR}/dox/Utilities/VTKPythonWrapping/Executable\" \\
        \"${CMAKE_CURRENT_BINARY_DIR}/dox/Qt/Core\" \\
        \"${ParaView_SOURCE_DIR}/Qt/Core\" \\
        \"${ParaView_SOURCE_DIR}/Qt/Components\" \\
        \"${ParaView_SOURCE_DIR}/Qt/ApplicationComponents\" \\
        \"${ParaView_SOURCE_DIR}/Qt/Python\" \\
        \"${ParaView_SOURCE_DIR}/Qt/Widgets\" \\
        \"${ParaView_SOURCE_DIR}/Qt/Testing\" \\
        \"${ParaView_SOURCE_DIR}/CoProcessing/CoProcessor\" \\
        \"${ParaView_SOURCE_DIR}/Applications/ParaView\" \\
        \"${CMAKE_CURRENT_BINARY_DIR}/dox/Common/KWCommon\" \\
        \"${CMAKE_CURRENT_BINARY_DIR}/dox/ParaViewCore/Common\" \\
        \"${CMAKE_CURRENT_BINARY_DIR}/dox/ParaViewCore/ClientServerCore\" \\
        \"${CMAKE_CURRENT_BINARY_DIR}/dox/ParaViewCore/ServerImplementation\" \\
        \"${CMAKE_CURRENT_BINARY_DIR}/dox/ParaViewCore/ServerManager\" \\
        \"${CMAKE_CURRENT_BINARY_DIR}/dox/ParaViewCore/VTKExtensions\"")

  SET(DOXYGEN_IGNORE_PREFIX "vtkPV vtkSM vtk pq vtkCP")

  #
  # Configure the script and the doxyfile, then add target
  #
  IF(NOT DOT_PATH)
    GET_FILENAME_COMPONENT(DOT_PATH ${DOT} PATH)
  ENDIF(NOT DOT_PATH)

  SET(TAGFILES "\"${CMAKE_CURRENT_BINARY_DIR}/vtkNightlyDoc.tag=http://www.vtk.org/doc/nightly/html\"")

  IF(EXISTS ${QT_TAG_FILE})
    MESSAGE(STATUS "Including Qt documentation")
    SET (TAGFILES "${TAGFILES} \"${QT_TAG_FILE}=http://doc.trolltech.com/4.6/\"")
  ENDIF(EXISTS ${QT_TAG_FILE})

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
