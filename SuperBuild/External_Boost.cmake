# The Boost external project for Titan

set(boost_source  "${CMAKE_CURRENT_BINARY_DIR}/Boost")
set(boost_binary  "${CMAKE_CURRENT_BINARY_DIR}/Boost-build")
set(boost_install  "${CMAKE_CURRENT_BINARY_DIR}")

if(MSVC)
  set(boost_lib_args
    -DENABLE_SHARED:BOOL=OFF
    -DENABLE_STATIC:BOOL=ON
  )
else()
  set(boost_lib_args
    -DENABLE_SHARED:BOOL=ON
    -DENABLE_STATIC:BOOL=OFF
  )
endif()

ExternalProject_Add(Boost
  DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
  URL ${BOOST_URL}/${BOOST_GZ}
  URL_MD5 ${BOOST_MD5}
  UPDATE_COMMAND ""
  SOURCE_DIR ${boost_source}
  BINARY_DIR ${boost_binary}
  CMAKE_ARGS
     ${boost_lib_args}
     ${pv_tpl_compiler_args}
    -DBUILD_SHARED_LIBS:BOOL=ON
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG:PATH=${boost_binary}/lib
    -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG:PATH=${boost_binary}/lib
    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE:PATH=${boost_binary}/lib
    -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE:PATH=${boost_binary}/lib
    -DBUILD_EXAMPLES:BOOL=OFF
    -DBUILD_TESTING:BOOL=OFF
    -DBUILD_VERSIONED:BOOL=OFF
    -DINSTALL_VERSIONED:BOOL=ON
    -DWINMANGLE_LIBNAMES:BOOL=ON
    -DWITH_MPI:BOOL=OFF
    -DWITH_PYTHON:BOOL=OFF
    -DCMAKE_INSTALL_PREFIX:PATH=${boost_install}
    ${Boost_EXTRA_ARGS}
  )

  ExternalProject_Add_Step(Boost InstallUUIDInclude
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${boost_source}/src/uuid/include/boost/uuid ${boost_install}/include/boost/uuid
    DEPENDEES install
  )

# These variables are used to find Boost by other projects
set(Boost_INCLUDE_DIR "${boost_install}/include" CACHE PATH "" FORCE)
set(BOOST_LIBRARYDIR "${boost_install}/lib" CACHE PATH "" FORCE)
