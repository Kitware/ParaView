# This file sets up include directories, link directories, and
# compiler settings for a project to use ParaView.  It should not be
# included directly, but rather through the ParaView_USE_FILE setting
# obtained from ParaViewConfig.cmake.

if(PARAVIEW_USE_FILE_INCLUDED)
  return()
endif()
set(PARAVIEW_USE_FILE_INCLUDED 1)

# Update CMAKE_MODULE_PATH so includes work.
list(APPEND CMAKE_MODULE_PATH ${ParaView_CMAKE_DIR})
include("${VTK_USE_FILE}")

if (PARAVIEW_ENABLE_QT_SUPPORT)
  set(QT_QMAKE_EXECUTABLE ${PARAVIEW_QT_QMAKE_EXECUTABLE})
  find_package(Qt4)
  if (QT4_FOUND)
    include("${QT_USE_FILE}")
  endif()
endif()

# FIXME: there was additional stuff about coprocessing and visit bridge here.
