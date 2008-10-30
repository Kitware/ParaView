INCLUDE(${QT_USE_FILE})


INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}
                    ${PARAVIEW_INCLUDE_DIRS}
                    ${VTK_INCLUDE_DIR}
                    ${pqCore_BINARY_DIR}
                    ${pqCore_SOURCE_DIR}
                    ${pqComponents_BINARY_DIR}
                    ${pqComponents_SOURCE_DIR})

ADD_PARAVIEW_PLUGIN(CosmoFiltersSMPlugin "1.0"
                    GUI_RESOURCE_FILES CosmoFiltersResources.xml
                    SERVER_MANAGER_XML CosmoFilters.xml
                    SERVER_MANAGER_SOURCES
                    vtkCosmoHaloCountFilter.cxx
                    vtkCosmoHaloClassFilter.cxx
                    vtkCosmoHaloFinder.cxx 
                    vtkCosmoDistillerSTD.cxx
                    vtkCosmoHaloSorter.cxx
                    vtkCosmoCorrelater.cxx
                    vtkCosmoDistillerVTU.cxx)
             
QT4_WRAP_CPP(MOC_SRCS HaloClassPanel.h)

ADD_PARAVIEW_OBJECT_PANEL(IFACES IFACE_SRCS
                          CLASS_NAME HaloClassPanel
                          XML_NAME HaloClass
                          XML_GROUP filters)

ADD_PARAVIEW_PLUGIN(GUIHaloClassPanel "1.0"
                    GUI_INTERFACES ${IFACES}
                    SOURCES ${MOC_SRCS}
                            ${IFACE_SRCS}
                            HaloClassPanel.h
                            HaloClassPanel.cxx)
