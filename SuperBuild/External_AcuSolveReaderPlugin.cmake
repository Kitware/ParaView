
set(AcuSolveReaderPlugin_source "${CMAKE_CURRENT_BINARY_DIR}/AcuSolveReaderPlugin")

# create an external project to download yt,
# and configure and build it
ExternalProject_Add(AcuSolveReaderPlugin
  GIT_REPOSITORY "git://kwsource.kitwarein.com/paraview/acusolvereaderplugin.git"
  SOURCE_DIR ${AcuSolveReaderPlugin_source}
  BINARY_DIR ""
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  UPDATE_COMMAND ""
  INSTALL_COMMAND ""
  DEPENDS
    ${AcusolveReaderPlugin_dependencies}
  )
