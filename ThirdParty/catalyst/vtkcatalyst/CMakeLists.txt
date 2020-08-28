if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/catalyst/CMakeLists.txt")
  message(FATAL_ERROR
    "Catalyst requested, but the Catalyst submodule is not initialized. Please "
    "run 'git submodule update --init --recursive' in the source directory.")
endif()

set(CATALYST_BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS}")
set(CATALYST_BUILD_TESTING OFF)

# essential to turn of STUB implementation otherwise we may end up overwriting
# the ParaView Catalyst library.
set(CATALYST_BUILD_STUB_IMPLEMENTATION OFF)

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
    catalyst::core
    catalyst::conduit
    catalyst::blueprint)

set(_catalyst_macros_path "${CMAKE_CURRENT_SOURCE_DIR}/catalyst/cmake" CACHE INTERNAL "")
