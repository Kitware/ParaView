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

TARGET_LINK_LIBRARIES(GMVReader LINK_PRIVATE vtksys)

# Add testing if necessary
IF (BUILD_TESTING)
  ADD_SUBDIRECTORY(Testing)
ENDIF ()

if(PARAVIEW_USE_MPI)
  vtk_mpi_link(GMVReader)
endif()
