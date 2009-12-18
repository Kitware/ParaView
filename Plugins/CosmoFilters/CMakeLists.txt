INCLUDE_DIRECTORIES(${PARAVIEW_INCLUDE_DIRS}
                    ${VTK_INCLUDE_DIR})

SET(COSMO_FILTERS_SOURCES
#    vtkCosmoHaloCountFilter.cxx
#    vtkCosmoHaloClassFilter.cxx
    vtkCosmoHaloFinder.cxx 
    vtkPCosmoHaloFinder.cxx
#    vtkCosmoDistillerSTD.cxx
#    vtkCosmoHaloSorter.cxx
#    vtkCosmoCorrelater.cxx
#    vtkCosmoDistillerVTU.cxx
)

SET(SERVER_ARGS
    SERVER_MANAGER_XML CosmoFilters.xml
    SERVER_MANAGER_SOURCES ${COSMO_FILTERS_SOURCES})    
    
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

#      QT4_WRAP_CPP(MOC_SRCS HaloClassPanel.h)

#      ADD_PARAVIEW_OBJECT_PANEL(IFACES IFACE_SRCS
#                                CLASS_NAME HaloClassPanel
#                                XML_NAME HaloClass
#                                XML_GROUP filters)

      SET(CLIENT_ARGS
          GUI_INTERFACES ${IFACES}
          GUI_RESOURCE_FILES CosmoFiltersResources.xml
          SOURCES ${IFACE_SRCS}
                  ${MOC_SRCS}
#                  HaloClassPanel.h
#                  HaloClassPanel.cxx
      )

    ENDIF(QT4_FOUND)
ENDIF(PARAVIEW_BUILD_QT_GUI)

ADD_PARAVIEW_PLUGIN(CosmoFiltersPlugin "1.0"
                    ${SERVER_ARGS}
                    ${CLIENT_ARGS}) 
