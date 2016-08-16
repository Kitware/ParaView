cmake_minimum_required(VERSION 3.3)

if (NOT ParaView_BINARY_DIR)
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})
endif()

# Set a consistent MACOSX_RPATH default across all CMake versions.
# When CMake 2.8.12 is required, change this default to 1.
# When CMake 3.0.0 is required, remove this block (see CMP0042).
if(NOT DEFINED CMAKE_MACOSX_RPATH)
  set(CMAKE_MACOSX_RPATH 0)
endif()

if (NOT TARGET vtkIOVisItBridge)
  message(FATAL_ERROR
    "Please rebuild ParaView with VisIt reader support enabled.")
endif()

FIND_PACKAGE(ParaView REQUIRED)
INCLUDE(${PARAVIEW_USE_FILE})

SET(SOURCES
avtFluentFileFormat.h
avtFluentFileFormat.C
)

ADD_VISIT_PLUGIN_READER(VisItReaderExample "1.0"
  VISIT_READER_NAME "avtFluentFileFormat"
  VISIT_READER_TYPE "STMD"
  VISIT_READER_FILE_PATTERN "cas"
  SERVER_SOURCES ${SOURCES}
  )

TARGET_LINK_LIBRARIES(VisItReaderExample vtkVisItAVTAlgorithms)
