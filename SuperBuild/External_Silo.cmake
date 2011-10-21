
# The Silo external project for ParaView
set(Silo_source "${CMAKE_CURRENT_BINARY_DIR}/Silo")
set(Silo_install "${CMAKE_CURRENT_BINARY_DIR}")

# If Windows we use CMake otherwise ./configure
if(WIN32)

  if("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
    set(silo_configuration "DllwithHDF5_Release|x64")
    set(silo_bin_dir SiloWindows/MSVC8/x64/DllwithHDF5_Release)
  else()
    set(silo_configuration "DllwithHDF5_Release|Win32")
    set(silo_bin_dir SiloWindows/MSVC8/Win32/DllwithHDF5_Release)
  endif()
  
  configure_file(${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/silo_patch_step.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/Silo_patch_step.cmake
    @ONLY)

  # run's copysilo.bat which generates silo.h
  configure_file(${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/silo_configure_step.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/Silo_configure_step.cmake
    @ONLY)

  configure_file(${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/silo_build_step.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/Silo_build_step.cmake
    @ONLY)

  set(Silo_PATCH_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/Silo_patch_step.cmake)
  set(Silo_CONFIGURE_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/Silo_configure_step.cmake)
  set(Silo_BUILD_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/Silo_build_step.cmake)
  set(Silo_INSTALL_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/Silo_install_step.cmake)

  ExternalProject_Add(Silo
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SOURCE_DIR ${Silo_source}
    INSTALL_DIR ${Silo_install}
    URL ${SILO_URL}/${SILO_GZ}
    URL_MD5 ${SILO_MD5}
    BUILD_IN_SOURCE 1
    UPDATE_COMMAND ""
    PATCH_COMMAND ${Silo_PATCH_COMMAND}
    CONFIGURE_COMMAND ${Silo_CONFIGURE_COMMAND}
    BUILD_COMMAND ${Silo_BUILD_COMMAND}
    INSTALL_COMMAND ""
    DEPENDS ${Silo_dependencies}
  )

  ExternalProject_Add_Step(Silo CopySiloLib
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${Silo_source}/${silo_bin_dir}/silohdf5.lib ${Silo_install}/lib/silohdf5.lib
    DEPENDEES install
  )

  ExternalProject_Add_Step(Silo CopySiloDll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${Silo_source}/${silo_bin_dir}/silohdf5.dll ${Silo_install}/bin/silohdf5.dll
    DEPENDEES install
  )

  ExternalProject_Add_Step(Silo CopySiloInclude
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${Silo_source}/SiloWindows/include ${Silo_install}/include
    DEPENDEES install
  )

else()

  if(QT_QMAKE_EXECUTABLE)
    get_filename_component(qt_bin_dir ${QT_QMAKE_EXECUTABLE} PATH)
    get_filename_component(qt_dir ${qt_bin_dir} PATH)
  endif()

  configure_file(${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/silo_patch_step.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/silo_patch_step.cmake
    @ONLY)
  
  configure_file(${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/silo_configure_step.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/silo_configure_step.cmake
    @ONLY)

  set(Silo_PATCH_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/silo_patch_step.cmake)
  set(Silo_CONFIGURE_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/silo_configure_step.cmake)

  ExternalProject_Add(Silo
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SOURCE_DIR ${Silo_source}
    INSTALL_DIR ${Silo_install}
    URL ${SILO_URL}/${SILO_GZ}
    URL_MD5 ${SILO_MD5}
    BUILD_IN_SOURCE 1
    PATCH_COMMAND ${Silo_PATCH_COMMAND}
    CONFIGURE_COMMAND ${Silo_CONFIGURE_COMMAND}
    DEPENDS ${Silo_dependencies}
  )

endif()

set(SILO_INCLUDE_DIR ${Silo_install}/include)

if(WIN32)
  set(SILO_LIBRARY ${Silo_install}/lib/silohdf5${_LINK_LIBRARY_SUFFIX})
else()
  set(SILO_LIBRARY ${Silo_install}/lib/libsiloh5${_LINK_LIBRARY_SUFFIX})
endif()

set(Silo_DIR "${Silo_binary}" CACHE PATH "Silo binary directory" FORCE)
mark_as_advanced(Silo_DIR)
