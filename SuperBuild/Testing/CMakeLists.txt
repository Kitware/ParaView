
cmake_minimum_required(VERSION 2.8.3)

project(ParaViewBinaryTesting)

if(NOT Testing_Temporary_dir)
  set(Testing_Temporary_dir ${CMAKE_CURRENT_BINARY_DIR}/Testing/Temporary)
endif()

file(TO_NATIVE_PATH ${Testing_Temporary_dir} Testing_Temporary_Native_dir)
set(PV_INSTALL_LOCATION ${Testing_Temporary_dir}/install)
set(PV_INSTALL_TEST_XML_DIR ${ParaViewBinaryTesting_SOURCE_DIR}/XML)

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
    COMMAND yes Y | /usr/bin/hdiutil attach ${ParaView_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME}.dmg > /dev/null
  )

  add_test(
    NAME InstallParaView 
    COMMAND installer ${CPACK_PACKAGE_FILE_NAME}.dmg -target ${Testing_Temporary_Native_dir}
    WORKING_DIRECTORY /Volumes/${CPACK_PACKAGE_FILE_NAME}
  )

  add_test(
    NAME UnMountDMG 
    COMMAND /usr/bin/hdiutil detach /Volumes/${CPACK_PACKAGE_FILE_NAME}
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

if(PARAVIEW_DATA_ROOT)
  add_test(
    NAME LoadXDMFZlibData.xml
    COMMAND ${PV_INSTALL_BINARY} -dr --disable-light-kit --test-directory=${Testing_Temporary_dir} --test-script=${PV_INSTALL_TEST_XML_DIR}/LoadXDMFZlibData.xml --exit
  )

  if(PARAVIEW_BUILD_Silo)
    add_test(
      NAME LoadSiloData.xml
      COMMAND ${PV_INSTALL_BINARY} -dr --disable-light-kit --test-directory=${Testing_Temporary_dir} --test-script=${PV_INSTALL_TEST_XML_DIR}/LoadXDMFZlibData.xml --exit
    )
  endif()

  if(PARAVIEW_BUILD_CGNS)
    add_test(
      NAME LoadCGNSData.xml
      COMMAND ${PV_INSTALL_BINARY} -dr --disable-light-kit --test-directory=${Testing_Temporary_dir} --test-script=${PV_INSTALL_TEST_XML_DIR}/LoadXDMFZlibData.xml --exit
    )
  endif()
endif()

# On Windows run Uninstall
if(WIN32)
  add_test(
    NAME UninstallParaView 
    COMMAND ${PV_INSTALL_LOCATION}/Uninstall.exe /S
    WORKING_DIRECTORY ${PV_INSTALL_LOCATION}
  )
endif()
