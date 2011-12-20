
message("Building tests!!")

ExternalProject_Get_Property(ParaView binary_dir)
set(ParaView_BINARY_DIR "${binary_dir}")

#include cpack file to get package variables
include(${ParaView_BINARY_DIR}/Applications/ParaView/CPackParaViewConfig.cmake)

file(TO_NATIVE_PATH ${Testing_Temporary_dir} Testing_Temporary_Native_dir)

# First order of business. Install ParaView!
if(WIN32)
  # we need the short path to the temp dir
  execute_process(
    COMMAND cscript /NoLogo ${ParaViewSuperBuild_SOURCE_DIR}/shortpath.vbs ${Testing_Temporary_Native_dir}
    OUTPUT_VARIABLE Testing_Temporary_short_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  add_test(
    NAME InstallParaView
    COMMAND ${ParaView_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME}.exe /S /NCRC /D=${Testing_Temporary_short_dir}\\install
  )
elseif(UNIX AND NOT APPLE)
  add_test(
    NAME InstallParaView 
    COMMAND ${CMAKE_COMMAND} -E tar xzf ${CPACK_PACKAGE_FILE_NAME}.tar.gz 
    WORKING_DIRECTORY ${Testing_Temporary_dir}
  )
else()
  add_test(InstallParaView ${ParaView_Package})
endif()