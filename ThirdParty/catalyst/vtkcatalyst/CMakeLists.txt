if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/catalyst/CMakeLists.txt")
  message(FATAL_ERROR
    "Catalyst requested, but the Catalyst submodule is not initialized. Please "
    "run 'git submodule update --init --recursive' in the source directory.")
endif()

# XXX(catalyst-static): Catalyst does not implement the logic to build and
# import plugins statically yet. Force shared libraries for now.
#set(CATALYST_BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS}")
set(CATALYST_BUILD_SHARED_LIBS ON)
set(CATALYST_BUILD_TESTING OFF)

# Always ship the stub implementation.
set(CATALYST_BUILD_STUB_IMPLEMENTATION ON)

set(CATALYST_BUILD_REPLAY OFF CACHE INTERNAL "")

# Update install locations
set(CATALYST_INSTALL_LIBRARY_DIR "${_vtk_build_LIBRARY_DESTINATION}")
set(CATALYST_INSTALL_ARCHIVE_DIR "${_vtk_build_ARCHIVE_DESTINATION}")
set(CATALYST_INSTALL_INCLUDE_DIR "${_vtk_build_HEADERS_DESTINATION}/catalyst")
set(CATALYST_INSTALL_PACKAGE_DIR "${_vtk_build_CMAKE_DESTINATION}/catalyst")

add_subdirectory(catalyst)

vtk_module_add_module(ParaView::vtkcatalyst
  HEADER_ONLY)
vtk_module_link(ParaView::vtkcatalyst
  INTERFACE
    catalyst::catalyst)

set(_catalyst_macros_path "${CMAKE_CURRENT_SOURCE_DIR}/catalyst/cmake" CACHE INTERNAL "")
