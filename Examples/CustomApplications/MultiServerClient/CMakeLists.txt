#=========================================================================
#
#   Program: ParaView
#   Module:  CMakeLists.txt
#
#   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
#   All rights reserved.
#
#   ParaView is a free software; you can redistribute it and/or modify it
#   under the terms of the ParaView license version 1.2. 
#   
#   See License_v1.2.txt for the full ParaView license.
#   A copy of this license can be obtained by contacting
#   Kitware Inc.
#   28 Corporate Drive
#   Clifton Park, NY 12065
#   USA
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
#CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#========================================================================
cmake_minimum_required(VERSION 3.3)
project(MultiServerClient)

find_package(ParaView REQUIRED)
include(${PARAVIEW_USE_FILE})
if (NOT PARAVIEW_BUILD_QT_GUI)
  message(FATAL_ERROR
    "MultiServerClient example requires PARAVIEW_BUILD_QT_GUI to be enabled. "
    "Please rebuild ParaView (or point to a different build of ParaView) "
    "with PARAVIEW_BUILD_QT_GUI set to TRUE")
endif()

# Set a consistent MACOSX_RPATH default across all CMake versions.
# When CMake 2.8.12 is required, change this default to 1.
# When CMake 3.0.0 is required, remove this block (see CMP0042).
if(NOT DEFINED CMAKE_MACOSX_RPATH)
  set(CMAKE_MACOSX_RPATH 0)
endif()

SET (SOURCE_FILES
  MultiServerClientMainWindow.cxx
  MultiServerClientMainWindow.h
)

if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
  QT5_WRAP_CPP(MOC_SRCS
    MultiServerClientMainWindow.h
  )
  QT5_WRAP_UI(UI_BUILT_SOURCES
    MultiServerClientMainWindow.ui
  )
else ()
  QT4_WRAP_CPP(MOC_SRCS
    MultiServerClientMainWindow.h
  )
  QT4_WRAP_UI(UI_BUILT_SOURCES
    MultiServerClientMainWindow.ui
  )
endif ()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

#------------------------------------------------------------------------------
# Build the client
include(ParaViewBranding)

build_paraview_client(multiserverclient
    TITLE "Multi-Server Client (ParaView-based Example Application)"
    ORGANIZATION  "Kitware Inc"
    VERSION_MAJOR 1
    VERSION_MINOR 0
    VERSION_PATCH 0
    PVMAIN_WINDOW MultiServerClientMainWindow 
    PVMAIN_WINDOW_INCLUDE MultiServerClientMainWindow.h
    SOURCES ${SOURCE_FILES} ${MOC_SRCS} ${UI_BUILT_SOURCES}
    GUI_CONFIGURATION_XMLS ${CMAKE_CURRENT_SOURCE_DIR}/Sources.xml
)
