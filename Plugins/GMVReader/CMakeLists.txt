OPTION(GMVReader_SKIP_DATARANGE_CALCULATIONS_IN_REQUEST_INFORMATION_CALLS
  "Calculate minimum and maximum of data arrays in RequestInformation calls.
   A feature inherited from AVSucdReader, but it seems this information is
   never queried by ParaView."
  OFF)
MARK_AS_ADVANCED(GMVReader_SKIP_DATARANGE_CALCULATIONS_IN_REQUEST_INFORMATION_CALLS)
IF(GMVReader_SKIP_DATARANGE_CALCULATIONS_IN_REQUEST_INFORMATION_CALLS)
  SET_SOURCE_FILES_PROPERTIES(
    vtkGMVReader.cxx
    COMPILE_FLAGS -DGMVREADER_SKIP_DATARANGE_CALCULATIONS)
ENDIF()

#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
INCLUDE_DIRECTORIES(${VTK_INCLUDE_DIRS})

# -----------------------------------------------------------------------------
# Where are gmvread.c, gmvread.h and gmvrayread.h stored?
# -----------------------------------------------------------------------------
SET(GMVReader_GMVREAD_LIB_DIR "GMVREAD-NOTFOUND" CACHE PATH "The directory containing gmvread.c, gmvread.h and gmvrayread.h")
MARK_AS_ADVANCED(GMVReader_GMVREAD_LIB_DIR)
FIND_PATH(GMVReader_GMVREAD_LIB_DIR
  NAMES gmvread.c gmvread.h gmvrayread.h
  PATHS ${vtkIOVisItBridge_INCLUDE_DIRS} ${ParaView_SOURCE_DIR}/Utilities/VisItBridge/databases
  PATH_SUFFIXES GMV
  DOC "The directory containing gmvread.c, gmvread.h and gmvrayread.h"
  )
IF (GMVReader_GMVREAD_LIB_DIR)
  INCLUDE_DIRECTORIES("${GMVReader_GMVREAD_LIB_DIR}")
  INCLUDE_DIRECTORIES("${ParaView_SOURCE_DIR}/Utilities/VisItBridge/Library/VisItLib/common/utility")
ELSE ()
  MESSAGE(FATAL_ERROR "Please set GMVReader_GMVREAD_LIB_DIR to where gmvread.c, gmvread.h and gmvrayread.h in the VisItBridge source tree can be found.")
ENDIF ()

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
  ENDIF ()
ELSEIF (UNIX)
  IF (CMAKE_COMPILER_IS_GNUCXX)
    # -------------------------------------------------------------------------
    # Disable "ignoring return value of ..." warnings from included gmvread.c
    # -------------------------------------------------------------------------
    INCLUDE(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG(-Wno-unused-result Wno-unused-result)
    IF (Wno-unused-result)
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-result")
    ENDIF ()
  ENDIF ()
ENDIF ()

if(PARAVIEW_USE_MPI)
  include(vtkMPI)
endif()

ADD_PARAVIEW_PLUGIN(GMVReader "1.0"
  REQUIRED_ON_SERVER
  SERVER_MANAGER_XML GMVReaderSM.xml
  SERVER_MANAGER_SOURCES vtkGMVReader.cxx)

# Add testing if necessary
IF (BUILD_TESTING)
  ADD_SUBDIRECTORY(Testing)
ENDIF ()

if(PARAVIEW_USE_MPI)
  vtk_mpi_link(GMVReader)
endif()
