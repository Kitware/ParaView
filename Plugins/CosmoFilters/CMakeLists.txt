INCLUDE_DIRECTORIES(${PARAVIEW_INCLUDE_DIRS}
                    ${VTK_INCLUDE_DIR})



IF(VTK_USE_MPI)
INCLUDE(FindMPI)

INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})

ADD_DEFINITIONS("-DMPICH_IGNORE_CXX_SEEK")
ELSE()

ADD_DEFINITIONS("-DUSE_SERIAL_COSMO")

ENDIF()



INCLUDE(CheckIncludeFiles)
CHECK_INCLUDE_FILES("dirent.h" HAVE_DIRENT_H)

IF(NOT (HAVE_DIRENT_H OR WIN32))
  MESSAGE("Error: Can't find dirent.h for the Cosmo plugin.")
ENDIF(NOT (HAVE_DIRENT_H OR WIN32))



IF(HAVE_DIRENT_H OR WIN32)

ADD_DEFINITIONS("-DUSE_VTK_COSMO")

SET(COSMO_FILTERS_SOURCES
    vtkPCosmoReader.cxx
    vtkPCosmoHaloFinder.cxx
)

SET(RRU_SOURCES
    Partition.cxx
    ParticleDistribute.cxx
    ParticleExchange.cxx
    Message.cxx
    CosmoHaloFinder.cxx
    CosmoHaloFinderP.cxx
    FOFHaloProperties.cxx
)

SET(SERVER_ARGS
    SERVER_MANAGER_XML CosmoFilters.xml
    SERVER_MANAGER_SOURCES ${COSMO_FILTERS_SOURCES}
)    
    
IF(PARAVIEW_BUILD_QT_GUI)
   FIND_PACKAGE(Qt4)
   IF(QT4_FOUND)
      INCLUDE(${QT_USE_FILE})

      INCLUDE_DIRECTORIES(${PARAVIEW_INCLUDE_DIRS}
                          ${VTK_INCLUDE_DIR}
                          ${pqCore_BINARY_DIR}
                          ${pqCore_SOURCE_DIR}
                          ${pqComponents_BINARY_DIR}
                          ${pqComponents_SOURCE_DIR})

      SET(CLIENT_ARGS
          GUI_INTERFACES ${IFACES}
          GUI_RESOURCE_FILES CosmoFiltersResources.xml CosmoReaderResources.xml
          SOURCES ${RRU_SOURCES}
                  ${IFACE_SRCS}
                  ${MOC_SRCS}
      )
   ELSE()
      SET(CLIENT_ARGS
          SOURCES ${RRU_SOURCES}
      )
   ENDIF()
ELSE()
   SET(CLIENT_ARGS
       SOURCES ${RRU_SOURCES}
   )
ENDIF()

ADD_PARAVIEW_PLUGIN(CosmoFiltersPlugin "1.0"
                    ${SERVER_ARGS}
                    ${CLIENT_ARGS}) 

ENDIF(HAVE_DIRENT_H OR WIN32)
