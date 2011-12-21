
ExternalProject_Get_Property(ParaView binary_dir)
set(ParaView_BINARY_DIR "${binary_dir}")

#include cpack file to get package variables
include(${ParaView_BINARY_DIR}/Applications/ParaView/CPackParaViewConfig.cmake)

file(TO_NATIVE_PATH ${Testing_Temporary_dir} Testing_Temporary_Native_dir)

set(PV_INSTALL_LOCATION ${Testing_Temporary_dir}/install)
set(PV_INSTALL_TEST_XML_DIR ${ParaViewSuperBuild_SOURCE_DIR}/Testing/XML)

# First order of business. Install ParaView!
if(WIN32)
  # we need the short path to the temp dir
  execute_process(
    COMMAND cscript /NoLogo ${ParaViewSuperBuild_SOURCE_DIR}/shortpath.vbs ${Testing_Temporary_Native_dir}
    OUTPUT_VARIABLE Testing_Temporary_short_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  add_test(
    NAME InstallParaView
    COMMAND ${ParaView_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME}.exe /S /NCRC /D=${Testing_Temporary_short_dir}\\install
  )
elseif(UNIX AND NOT APPLE)
  make_directory(${PV_INSTALL_LOCATION}/install)
  set(PV_INSTALL_LOCATION ${Testing_Temporary_dir}/install/${CPACK_PACKAGE_FILE_NAME})
  
  add_test(
    NAME InstallParaView 
    COMMAND ${CMAKE_COMMAND} -E tar xzf ${ParaView_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME}.tar.gz 
    WORKING_DIRECTORY ${Testing_Temporary_dir}/install
  )
else()
  add_test(
    NAME MountDMG
    COMMAND hdiutil attach ${ParaView_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME}.dmg
  )

  add_test(
    NAME InstallParaView 
    COMMAND installer ${CPACK_PACKAGE_FILE_NAME}.dmg -target ${Testing_Temporary_Native_dir}
    WORKING_DIRECTORY /Volumes/${CPACK_PACKAGE_FILE_NAME}
  )

  add_test(
    NAME UnMountDMG 
    COMMAND hdiutil detach /Volumes/${CPACK_PACKAGE_FILE_NAME}.dmg
  )
endif()

set(PV_INSTALL_BINARY ${PV_INSTALL_LOCATION}/bin/paraview${CMAKE_EXECUTABLE_SUFFIX})

if(PARAVIEW_BUILD_PLUGIN_AcuSolveReader)
  add_test(
    NAME LoadAcuSolveReaderPlugin
    COMMAND ${PV_INSTALL_BINARY} -dr --disable-light-kit --test-directory=${Testing_Temporary_dir} --test-script=${PV_INSTALL_TEST_XML_DIR}/LoadAcuSolveReaderPlugin.xml --exit
  )
endif()

if(PARAVIEW_BUILD_PLUGIN_VisTrails)
  add_test(
    NAME LoadVisTrailsPlugin
    COMMAND ${PV_INSTALL_BINARY} -dr --disable-light-kit --test-directory=${Testing_Temporary_dir} --test-script=${PV_INSTALL_TEST_XML_DIR}/LoadVisTrailsReaderPlugin.xml --exit
  )
endif()

if(PARAVIEW_BUILD_PLUGIN_Nektar)
  add_test(
    NAME LoadNektarReaderPlugin
    COMMAND ${PV_INSTALL_BINARY} -dr --disable-light-kit --test-directory=${Testing_Temporary_dir} --test-script=${PV_INSTALL_TEST_XML_DIR}/LoadNektarReaderPlugin.xml --exit
  )
endif()
