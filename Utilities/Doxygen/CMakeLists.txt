#
# Build the documentation
#

INCLUDE (${CMAKE_ROOT}/Modules/Documentation.cmake)

IF (BUILD_DOCUMENTATION)
  OPTION (DOCUMENTATION_DOWNLOAD_VTK_TAGFILE
         "Download the VTK tag file." ON)
  MARK_AS_ADVANCED (
    DOCUMENTATION_DOWNLOAD_VTK_TAGFILE
  )

  SET(VTK_DOXYGEN_HOME "${ParaView_SOURCE_DIR}/VTK/Utilities/Doxygen")


  #
  # Configure the script and the doxyfile, then add target
  #
  CONFIGURE_FILE(
    ${ParaView_SOURCE_DIR}/Utilities/Doxygen/doxyfile.in
    ${ParaView_BINARY_DIR}/Utilities/Doxygen/doxyfile)

  CONFIGURE_FILE(
    ${ParaView_SOURCE_DIR}/Utilities/Doxygen/doc_makeall.sh.in
    ${ParaView_BINARY_DIR}/Utilities/Doxygen/doc_makeall.sh)

  ADD_CUSTOM_TARGET(ParaViewDoxygenDoc
    ${BASH}
    ${ParaView_BINARY_DIR}/Utilities/Doxygen/doc_makeall.sh)

ENDIF (BUILD_DOCUMENTATION)
