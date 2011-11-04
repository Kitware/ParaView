
# The CGNS external project for ParaView
set(CGNS_source "${CMAKE_CURRENT_BINARY_DIR}/CGNS")
set(CGNS_install "${CMAKE_CURRENT_BINARY_DIR}")

set(CGNS_INCLUDE_DIR "${CGNS_install}/include")
if(APPLE)
  set(CGNS_LIBRARY "${CGNS_install}/lib/libcgns.a")
else()
  set(CGNS_LIBRARY "${CGNS_install}/lib/libcgns${_LINK_LIBRARY_SUFFIX}")
endif()

# If Windows we use CMake otherwise ./configure
if(WIN32)
  
  if("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
    set(cgns_64 -64)
  else()
    set(cgns_64)
  endif()
  
  # we need the short path to zlib and hdf5
  execute_process(
    COMMAND cscript /NoLogo ${CMAKE_CURRENT_SOURCE_DIR}/shortpath.vbs ${zlib_install}
    OUTPUT_VARIABLE zlib_dos_short_path
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  execute_process(
    COMMAND cscript /NoLogo ${CMAKE_CURRENT_SOURCE_DIR}/shortpath.vbs ${HDF5_install}
    OUTPUT_VARIABLE hdf5_dos_short_path
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  # getting short path doesnt work on directories that dont exist
  file(MAKE_DIRECTORY "${CGNS_install}")
  execute_process(
    COMMAND cscript /NoLogo ${CMAKE_CURRENT_SOURCE_DIR}/shortpath.vbs ${CGNS_install}
    OUTPUT_VARIABLE cgns_install_dos_short_path
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  ExternalProject_Add(CGNS
    URL ${CGNS_URL}/${CGNS_GZ}
    URL_MD5 ${CGNS_MD5}
    SOURCE_DIR ${CGNS_source}
    BUILD_IN_SOURCE 1
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/CGNSPatches/cgnslib.h" "${CGNS_source}/cgnslib.h"
    CONFIGURE_COMMAND configure -install ${cgns_install_dos_short_path} -dll ${cgns_64} -zlib ${zlib_dos_short_path}
    BUILD_COMMAND nmake
    INSTALL_COMMAND nmake install
    DEPENDS ${CGNS_dependencies}
    )

elseif(APPLE)
  # cgns only appears to build statically on mac.

  # cgns install system sucks..
  file(MAKE_DIRECTORY "${CGNS_install}/lib")
  file(MAKE_DIRECTORY "${CGNS_install}/include")
  
  if("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
    set(cgns_64 --enable-64bit)
  else()
    set(cgns_64)
  endif()

  configure_file(${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/CGNS_configure_step.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/CGNS_configure_step.cmake
    @ONLY)

  set(CGNS_CONFIGURE_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/CGNS_configure_step.cmake)

  ExternalProject_Add(CGNS
    SOURCE_DIR ${CGNS_source}
    INSTALL_DIR ${CGNS_install}
    URL ${CGNS_URL}/${CGNS_GZ}
    URL_MD5 ${CGNS_MD5}
    BUILD_IN_SOURCE 1
    PATCH_COMMAND ""
    CONFIGURE_COMMAND ${CGNS_CONFIGURE_COMMAND}
    DEPENDS ${CGNS_dependencies}
  )

else()

  # cgns install system sucks..
  file(MAKE_DIRECTORY "${CGNS_install}/lib")
  file(MAKE_DIRECTORY "${CGNS_install}/include")

  ExternalProject_Add(CGNS
    SOURCE_DIR ${CGNS_source}
    INSTALL_DIR ${CGNS_install}
    URL ${CGNS_URL}/${CGNS_GZ}
    URL_MD5 ${CGNS_MD5}
    BUILD_IN_SOURCE 1
    PATCH_COMMAND ""
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --with-zlib=${ZLIB_LIBRARY} --enable-64bit --enable-shared=all --disable-static --without-fortran
    DEPENDS ${CGNS_dependencies}
  )

  # more cgns install suck
  if(NOT WIN32 AND NOT APPLE)
    ExternalProject_Add_Step(CGNS SetCGNSLibExecutable
      COMMAND chmod a+x ${CGNS_LIBRARY}
      DEPENDEES install
    )
  endif()

endif()
