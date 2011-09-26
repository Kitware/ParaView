
# The png external project for ParaView
set(png_source "${CMAKE_CURRENT_BINARY_DIR}/png")
set(png_build "${CMAKE_CURRENT_BINARY_DIR}/png-build")
set(png_install "${CMAKE_CURRENT_BINARY_DIR}/png-install")

  ExternalProject_Add(png
    URL ${PNG_URL}/${PNG_GZ}
    URL_MD5 ${PNG_MD5}
    UPDATE_COMMAND ""
    SOURCE_DIR ${png_source}
    BINARY_DIR ${png_build}
    INSTALL_DIR ${png_install}
    CMAKE_CACHE_ARGS
      -DCMAKE_CXX_FLAGS:STRING=${pv_tpl_cxx_flags}
      -DCMAKE_C_FLAGS:STRING=${pv_tpl_c_flags}
      -DCMAKE_BUILD_TYPE:STRING=${CMAKE_CFG_INTDIR}
      ${pv_tpl_compiler_args}
      -DZLIB_INCLUDE_DIR:STRING=${ZLIB_INCLUDE_DIR}
      -DZLIB_LIBRARY:STRING=${ZLIB_LIBRARY}
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    DEPENDS ${png_dependencies}
  )

set(PNG_INCLUDE_DIR ${png_install}/include)

if(CMAKE_CONFIGURATION_TYPES)
  set(PNG_LIBRARY optimized ${png_install}/lib/libpng${PNG_MAJOR}${PNG_MINOR}${_LINK_LIBRARY_SUFFIX} debug ${png_install}/lib/libpng${PNG_MAJOR}${PNG_MINOR}d${_LINK_LIBRARY_SUFFIX})
else()
  set(PNG_LIBRARY ${png_install}/lib/libpng${PNG_MAJOR}${PNG_MINOR}${_LINK_LIBRARY_SUFFIX})
endif()
