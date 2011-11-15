
# The freetype external project for ParaView
set(freetype_source "${CMAKE_CURRENT_BINARY_DIR}/freetype")
set(freetype_install "${CMAKE_CURRENT_BINARY_DIR}")

# If Windows we use CMake otherwise ./configure
if(WIN32)

  if("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
    set(freetype_configuration "LIB Release Multithreaded|x64")
  else()
    set(freetype_configuration "LIB Release Multithreaded|Win32")
  endif()
  set(freetype_dll_dir objs/release_mt)
  
  configure_file(${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/freetype_patch_step.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/freetype_patch_step.cmake
    @ONLY)

  configure_file(${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/freetype_build_step.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/freetype_build_step.cmake
    @ONLY)

  set(freetype_PATCH_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/freetype_patch_step.cmake)
  set(freetype_BUILD_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/freetype_build_step.cmake)

  ExternalProject_Add(freetype
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SOURCE_DIR ${freetype_source}
    INSTALL_DIR ${freetype_install}
    URL ${FT_URL}/${FT_GZ}
    URL_MD5 ${FT_MD5}
    BUILD_IN_SOURCE 1
    UPDATE_COMMAND ""
    PATCH_COMMAND ${freetype_PATCH_COMMAND}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ${freetype_BUILD_COMMAND}
    INSTALL_COMMAND ""
    DEPENDS ${freetype_dependencies}
  )

  ExternalProject_Add_Step(freetype CopyfreetypeLib
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${freetype_source}/${freetype_dll_dir}/freetype.lib ${freetype_install}/lib/freetype.lib
    DEPENDEES install
  )

  ExternalProject_Add_Step(freetype CopyfreetypeDll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${freetype_source}/${freetype_dll_dir}/freetype.dll ${freetype_install}/lib/freetype.dll
    DEPENDEES install
  )

  ExternalProject_Add_Step(freetype CopyfreetypeInclude
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${freetype_source}/include ${freetype_install}/include
    DEPENDEES install
  )

else()

  #configure_file(${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/freetype_patch_step.cmake.in
  #  ${CMAKE_CURRENT_BINARY_DIR}/freetype_patch_step.cmake
  #  @ONLY)
  
  #configure_file(${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/freetype_configure_step.cmake.in
  #  ${CMAKE_CURRENT_BINARY_DIR}/freetype_configure_step.cmake
  #  @ONLY)

  #set(freetype_PATCH_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/freetype_patch_step.cmake)
  #set(freetype_CONFIGURE_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/freetype_configure_step.cmake)

  ExternalProject_Add(freetype
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SOURCE_DIR ${freetype_source}
    INSTALL_DIR ${freetype_install}
    URL ${FT_URL}/${FT_GZ}
    URL_MD5 ${FT_MD5}
    BUILD_IN_SOURCE 1
    BUILD_IN_SOURCE 1
    PATCH_COMMAND ""
    CONFIGURE_COMMAND CFLAGS=-fPIC CXXFLAGS=-fPIC <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
    BUILD_COMMAND CFLAGS=-fPIC CXXFLAGS=-fPIC FCFLAGS=-fPIC make
    DEPENDS ${freetype_dependencies}
  )

endif()

set(FT_INCLUDE_DIR ${freetype_install}/include)

if(WIN32)
  set(FT_LIBRARY ${freetype_install}/lib/freetype${_LINK_LIBRARY_SUFFIX})
else()
  set(FT_LIBRARY ${freetype_install}/lib/libfreetype${_LINK_LIBRARY_SUFFIX})
endif()
