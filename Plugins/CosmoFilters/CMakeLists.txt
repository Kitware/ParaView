IF(NOT VTK_USE_MPI)
  MESSAGE("Error: MPI must be enabled for the Cosmo plugin to be built.")
ENDIF(NOT VTK_USE_MPI)

INCLUDE_DIRECTORIES(${PARAVIEW_INCLUDE_DIRS}
                    ${VTK_INCLUDE_DIR}
                    ${MPI_INCLUDE_PATH}
                    ${VTK_INCLUDE_DIRS_SYSTEM})

INCLUDE(CheckIncludeFiles)
CHECK_INCLUDE_FILES("stdint.h" HAVE_STDINT_H)
CHECK_INCLUDE_FILES("dirent.h" HAVE_DIRENT_H)

IF(NOT HAVE_STDINT_H)
  MESSAGE("Error: Cosmo plugin can't find stdint.h.")
ENDIF(NOT HAVE_STDINT_H)

IF(NOT HAVE_DIRENT_H)
  MESSAGE("Error: Cosmo plugin can't find dirent.h.")
ENDIF(NOT HAVE_DIRENT_H)

IF(VTK_USE_MPI AND HAVE_STDINT_H AND HAVE_DIRENT_H)

INCLUDE(FindMPI)

ADD_DEFINITIONS("-DUSE_VTK_COSMO -DMPICH_IGNORE_CXX_SEEK")

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

ENDIF(VTK_USE_MPI AND HAVE_STDINT_H AND HAVE_DIRENT_H)
