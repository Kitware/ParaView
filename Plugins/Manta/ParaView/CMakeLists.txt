PROJECT ( MantaPlugin )

#The manta plugin makes a ParaView View plugin which uses the vtkManta lib
#for data rendering inside the View.

# CMake version and policy issues
CMAKE_MINIMUM_REQUIRED( VERSION 2.4 )
IF( COMMAND CMAKE_POLICY )
  CMAKE_POLICY( SET CMP0003 NEW )
ENDIF( COMMAND CMAKE_POLICY )

#Find ParaView and get what we need from it ################################
IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${PARAVIEW_INCLUDE_DIRS}
    ${PARAVIEW_GUI_INCLUDE_DIRS}
    ${PARAVIEW_KWSYS_INCLUDE_DIRS}
    ${VTK_INCLUDE_DIR}
    ${MPI_INCLUDE_PATH}
    ${vtkManta_SOURCE_DIR}
    ${vtkManta_BINARY_DIR}
  )
ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)

INCLUDE ( ${PARAVIEW_USE_FILE} )

#Configure the plugin ######################################################
SET(MANTA_PLUGIN_SS_SOURCES
  vtkServerSideFactory.cxx
  vtkSMMantaBatchViewProxy.cxx
  vtkSMMantaClientServerViewProxy.cxx
  vtkSMMantaParallelViewProxy.cxx
  vtkSMMantaRepresentation.cxx
  vtkSMMantaViewProxy.cxx
  )      

SET(SERVER_ARGS 
  SERVER_MANAGER_XML MantaWrapping.xml 
  SERVER_MANAGER_SOURCES ${MANTA_PLUGIN_SS_SOURCES} )

IF(PARAVIEW_BUILD_QT_GUI)
  #set up client side pieces of the plugin
  
  INCLUDE(${QT_USE_FILE})
  
  SET(MANTA_PLUGIN_CS_SOURCES
    MantaDisplay.cxx
    MantaViewOptions.cxx
    pqMantaView.cxx
    )
      
  QT4_WRAP_CPP(MOC_SRCS 
    MantaDisplay.h
    MantaViewOptions.h 
    pqMantaView.h 
    )
  
  QT4_WRAP_UI(UI_SRCS
    MantaDisplay.ui
    MantaViewOptions.ui
    )
      
  ADD_PARAVIEW_VIEW_MODULE(
    VIEW_IFACE VIEW_IFACE_SRCS 
    VIEW_TYPE pqMantaView
    VIEW_NAME Manta          
    VIEW_XML_GROUP views
    VIEW_XML_NAME Manta
    )
  
  ADD_PARAVIEW_VIEW_OPTIONS(
    OPTIONS_IFACE OPTIONS_IFACE_SRCS
    VIEW_TYPE pqMantaView 
    GLOBAL_VIEW_OPTIONS MantaViewOptions
    )

  ADD_PARAVIEW_DISPLAY_PANEL_DECORATOR(
    DISPLAY_IFACE DISPLAY_IFACE_SRCS 
    CLASS_NAME MantaDisplay
    PANEL_TYPES pqDisplayProxyEditor)

  SET(CLIENT_ARGS
    GUI_INTERFACES 
    ${VIEW_IFACE} ${OPTIONS_IFACE} ${DISPLAY_IFACE}
    GUI_SOURCES 
    ${MANTA_PLUGIN_CS_SOURCES} ${MOC_SRCS} ${UI_SRCS}
    ${VIEW_IFACE_SRCS} ${OPTIONS_IFACE_SRCS} ${DISPLAY_IFACE_SRCS})
  
ENDIF(PARAVIEW_BUILD_QT_GUI)

#TODO: Is this really necessary?
LINK_DIRECTORIES ( ${MANTA_BUILD_LIB} )

ADD_PARAVIEW_PLUGIN ( MantaView "1.1" 
  ${SERVER_ARGS}
  ${CLIENT_ARGS}
  )

TARGET_LINK_LIBRARIES ( MantaView vtkMantaCS )

