# The FFMPEG external project for ParaView

set(ffmpeg_source "${CMAKE_CURRENT_BINARY_DIR}/FFMPEG")
set(ffmpeg_binary "${CMAKE_CURRENT_BINARY_DIR}/FFMPEG-build")
set(ffmpeg_install "${CMAKE_CURRENT_BINARY_DIR}/FFMPEG-install")

ExternalProject_Add(FFMPEG
  DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
  SOURCE_DIR ${ffmpeg_source}
  INSTALL_DIR ${ffmpeg_install}
  URL ${FFMPEG_URL}/${FFMPEG_GZ}
  URL_MD5 ${FFMPEG_MD5}
  BUILD_IN_SOURCE 1
  PATCH_COMMAND ""
  CONFIGURE_COMMAND <SOURCE_DIR>/configure --disable-static --disable-network --disable-zlib --disable-ffserver --disable-ffplay --disable-decoders --enable-shared --enable-swscale --prefix=<INSTALL_DIR>
  INSTALL_COMMAND ""
  )
