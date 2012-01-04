
set(VisTrails_source "${CMAKE_CURRENT_BINARY_DIR}/VisTrails")

# create an external project to download yt,
# and configure and build it
ExternalProject_Add(VisTrails
  URL ${VISTRAILS_URL}/${VISTRAILS_GZ}
  URL_MD5 ${VISTRAILS_MD5}
  DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
  SOURCE_DIR ${VisTrails_source}
  BINARY_DIR ""
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  UPDATE_COMMAND ""
  INSTALL_COMMAND ""
  DEPENDS
    ${VISTRAILS_dependencies}
  )
