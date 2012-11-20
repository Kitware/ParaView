PROJECT(GMVReader)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

OPTION(GMVReader_SKIP_DATARANGE_CALCULATIONS_IN_REQUEST_INFORMATION_CALLS
  "Calculate minimum and maximum of data arrays in RequestInformation calls.
   A feature inherited from AVSucdReader, but it seems this information is
   never queried by ParaView."
  OFF)
IF(GMVReader_SKIP_DATARANGE_CALCULATIONS_IN_REQUEST_INFORMATION_CALLS)
  SET_SOURCE_FILES_PROPERTIES(
    vtkGMVReader.cxx
    COMPILE_FLAGS -DGMVREADER_SKIP_DATARANGE_CALCULATIONS)
ENDIF(GMVReader_SKIP_DATARANGE_CALCULATIONS_IN_REQUEST_INFORMATION_CALLS)

#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
IF (ParaView_SOURCE_DIR)
  IF(PARAVIEW_BUILD_QT_GUI)
    INCLUDE(${QT_USE_FILE})
    INCLUDE_DIRECTORIES(
      ${PARAVIEW_GUI_INCLUDE_DIRS}
    )
  ENDIF(PARAVIEW_BUILD_QT_GUI)
  INCLUDE_DIRECTORIES(
    ${PARAVIEW_INCLUDE_DIRS}
    ${PARAVIEW_KWSYS_INCLUDE_DIRS}
    ${VTK_INCLUDE_DIRS}
  )

ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)


# -----------------------------------------------------------------------------
# Where are gmvread.c, gmvread.h and gmvrayread.h stored?
# -----------------------------------------------------------------------------
SET(GMVReader_GMVREAD_LIB_DIR "GMVREAD-NOTFOUND" CACHE PATH "The directory containing gmvread.c, gmvread.h and gmvrayread.h")
FIND_PATH(GMVReader_GMVREAD_LIB_DIR
  NAMES gmvread.c gmvread.h gmvrayread.h
  PATHS ${vtkIOVisItBridge_INCLUDE_DIRS} ${ParaView_SOURCE_DIR}/Utilities/VisItBridge/databases
  PATH_SUFFIXES GMV
  DOC "The directory containing gmvread.c, gmvread.h and gmvrayread.h"
  )
IF (GMVReader_GMVREAD_LIB_DIR)
  INCLUDE_DIRECTORIES("${GMVReader_GMVREAD_LIB_DIR}")
ELSE ()
  MESSAGE(FATAL_ERROR "Please set GMVReader_GMVREAD_LIB_DIR to where gmvread.c, gmvread.h and gmvrayread.h in the VisItBridge source tree can be found.")
ENDIF (GMVReader_GMVREAD_LIB_DIR)


# -----------------------------------------------------------------------------
# Add an option for building shared - default to the same choice made for VTK
# BUILD_SHARED_LIBS is a builtin cmake variable.
# -----------------------------------------------------------------------------
SET(BUILD_SHARED_LIBS ${PARAVIEW_BUILD_SHARED_LIBS})


# -----------------------------------------------------------------------------
# Disable some warnings
# -----------------------------------------------------------------------------
IF (WIN32)
  IF (MSVC_VERSION GREATER 1400)
    # -------------------------------------------------------------------------
    # Disable deprecation warnings for standard C and STL functions in VS2005
    # and newer
    # -------------------------------------------------------------------------
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
  ENDIF (MSVC_VERSION GREATER 1400)
ELSEIF (UNIX)
  IF (CMAKE_COMPILER_IS_GNUCXX)
    # -------------------------------------------------------------------------
    # Disable "ignoring return value of ..." warnings from included gmvread.c
    # -------------------------------------------------------------------------
    INCLUDE(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG(-Wno-unused-result Wno-unused-result)
    IF (Wno-unused-result)
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-result")
    ENDIF (Wno-unused-result)
  ENDIF (CMAKE_COMPILER_IS_GNUCXX)
ENDIF (WIN32)


# -----------------------------------------------------------------------------
# Determine suitable servermanager XML file
# -----------------------------------------------------------------------------
IF ("${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}.${PARAVIEW_VERSION_PATCH}" VERSION_GREATER "3.11.0")
  # -----------------------------------------------------------------------------
  # ParaView sources contain merely from 2011-Apr-08 onwards "Collaboration code"
  # which introduced a different servermanager XML requirement.
  SET(SM_XML GMVReaderSM.xml)


ELSE ("${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}.${PARAVIEW_VERSION_PATCH}")
  # -----------------------------------------------------------------------------
  # Be aware that ParaView sources contain merely from 2009-Sep-21 onwards
  # a version of vtkFileSeriesReader that can deal with file series where
  # multiple files provide the same time step value, e.g. constant zero for
  # all files. Adaptions to the XML server manager code are also required
  # for pre-CollaborationMerge source tree.
  # -----------------------------------------------------------------------------
  MESSAGE(FATAL_ERROR "Only ParaView 3.11.0 or higher supported.")

ENDIF ("${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}.${PARAVIEW_VERSION_PATCH}" VERSION_GREATER "3.11.0")

# ---------------------------------------------------------------------------
# Combined plugin
# -----------------------------------------------------------------------------
IF (PARAVIEW_BUILD_QT_GUI)
  # Extend the auto-generated panel
  QT4_WRAP_CPP(MOC_SRCS pqGMVReaderPanel.h)
  ADD_PARAVIEW_OBJECT_PANEL(IFACES IFACE_SRCS
    CLASS_NAME pqGMVReaderPanel
    XML_NAME GMVSeriesReader # name of SourceProxy in ${SM_XML}
    XML_GROUP sources
    )

  ADD_PARAVIEW_PLUGIN(GMVReaderPlugin "1.0"
    REQUIRED_ON_SERVER
    REQUIRED_ON_CLIENT
    DOCUMENTATION_DIR "${CMAKE_CURRENT_SOURCE_DIR}/doc"
    SERVER_MANAGER_XML ${SM_XML}
    SERVER_MANAGER_SOURCES vtkGMVReader.cxx
    GUI_RESOURCE_FILES GMVReaderGUI.xml
    GUI_INTERFACES ${IFACES}
    GUI_SOURCES ${MOC_SRCS} ${UI_SRCS} ${IFACE_SRCS} pqGMVReaderPanel.cxx
    )
ELSE (PARAVIEW_BUILD_QT_GUI)
  ADD_PARAVIEW_PLUGIN(GMVReaderPlugin "1.0"
    REQUIRED_ON_SERVER
    REQUIRED_ON_CLIENT
    DOCUMENTATION_DIR "${CMAKE_CURRENT_SOURCE_DIR}/doc"
    SERVER_MANAGER_XML ${SM_XML}
    SERVER_MANAGER_SOURCES vtkGMVReader.cxx)
ENDIF (PARAVIEW_BUILD_QT_GUI)
