cmake_minimum_required(VERSION 3.8)
project(ServerManagerRemoteConnection)

find_package(ParaView REQUIRED)

include(GNUInstallDirs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

add_executable(ServerManagerRemoteConnectionExample main.cxx)
target_link_libraries(ServerManagerRemoteConnectionExample PRIVATE
    ParaView::RemotingCore
    ParaView::RemotingApplication
    ParaView::RemotingViews)

enable_testing()
option(BUILD_TESTING "Build Server Manager Remote Connection test" OFF)

if(BUILD_TESTING)
    add_subdirectory(Testing)
endif()
