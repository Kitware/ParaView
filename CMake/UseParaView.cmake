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

# VTK_USE_FILE adds definitions for ${module}_AUTOINIT for all enabled modules.
# This is okay for VTK, with ParaView, AUTOINIT is not useful since one needs to
# init the CS streams separately. Also use AUTOINIT is defined, any
# application needs to link against all enabled VTK modules which ends up being
# a very long list and hard to keep up-to-date. We over come this issue by
# simply not setting the ${module}_AUTOINIT definies.
# So we get the current COMPILE_DEFINITIONS on the directory and remove
# references to AUTOINIT.
get_property(cur_compile_definitions DIRECTORY PROPERTY COMPILE_DEFINITIONS)
set (new_compile_definition)
foreach (defn IN LISTS cur_compile_definitions)
  string(REGEX MATCH "_AUTOINIT=" out-var "${defn}")
  if (NOT out-var)
    list(APPEND new_compile_definition "${defn}")
  endif()
endforeach()
set_property(DIRECTORY PROPERTY COMPILE_DEFINITIONS ${new_compile_definition})

# Import some commonly used cmake modules
include (ParaViewMacros)
include (ParaViewPlugins)
include (ParaViewBranding)
include (ParaViewQt)

if(PARAVIEW_ENABLE_QT_SUPPORT)
  if(PARAVIEW_QT_VERSION VERSION_GREATER "4")
    # nothing to do. the module system handles it properly.
  else()
    set(QT_QMAKE_EXECUTABLE "${PARAVIEW_QT_QMAKE_EXECUTABLE}" CACHE FILEPATH "Qt4 qmake executable")
    pv_find_package_qt(__tmp_qt_targets QT4_COMPONENTS QtGui)
    unset(__tmp_qt_targets)
  endif()
endif()

# Workaround for MPICH bug that produces error messages:
# "SEEK_SET is #defined but must not be for the C++ binding of MPI.
if (PARAVIEW_USE_MPI)
  add_definitions("-DMPICH_IGNORE_CXX_SEEK")
endif()

# FIXME: there was additional stuff about coprocessing and visit bridge here.
