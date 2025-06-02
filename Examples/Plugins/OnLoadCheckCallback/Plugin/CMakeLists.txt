# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-License-Identifier: BSD-3-Clause
set(qt_version_name "Qt${PARAVIEW_QT_MAJOR_VERSION}")

find_package(${qt_version_name} REQUIRED COMPONENTS Widgets)

paraview_add_plugin(OnLoadCheckCallback
  VERSION "1.0"
  SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/OnLoadCheckCallback.cxx")

target_link_libraries(OnLoadCheckCallback PRIVATE ${qt_version_name}::Widgets ParaView::pqCore)

option(BUILD_TESTING "Build Plugin Testing" OFF)
if (BUILD_TESTING AND BUILD_SHARED_LIBS)
  add_subdirectory(Testing)
endif()
