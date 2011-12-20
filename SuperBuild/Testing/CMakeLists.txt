
message("Building tests!!")

ExternalProject_Get_Property(ParaView binary_dir)
set(ParaView_BINARY_DIR "${binary_dir}")

#include cpack file to get package variables
include(${ParaView_BINARY_DIR}/Applications/ParaView/CPackParaViewConfig.cmake)

if(WIN32)
  add_test(InstallParaView "${ParaView_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME}.exe" /S /NCRC /D="${Testing_Temporary_dir}")
elseif(UNIX AND NOT APPLE)
  add_test(InstallParaView ${CMAKE_COMMAND} -E tar ${CPACK_PACKAGE_FILE_NAME}.tar.gz -C ${Testing_Temporary_dir})
else()
  add_test(InstallParaView ${ParaView_Package})
endif()