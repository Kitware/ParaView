PROJECT(NetDMFReaderPlugin)

#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${PARAVIEW_INCLUDE_DIRS}
    ${PARAVIEW_GUI_INCLUDE_DIRS}
    ${PARAVIEW_KWSYS_INCLUDE_DIRS}
  )
ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)

INCLUDE(${QT_USE_FILE})

# Must Have NetDMF
FIND_PACKAGE(NetDMF NO_MODULE REQUIRED)
IF(NetDMF_FOUND)

  INCLUDE_DIRECTORIES(
    ${VTK_INCLUDE_DIR}
    ${PARAVIEW_INCLUDE_DIRS}
    ${NETDMF_INCLUDE_DIRS}
    )
  IF(NETDMF_BUILD_MPI)
    INCLUDE_DIRECTORIES(
      ${NETDMF_MPI_INCLUDE_PATH})
  ENDIF(NETDMF_BUILD_MPI)

  LINK_DIRECTORIES(${NETDMF_LIBRARY_DIRS})

  SET( NetDmf_SRCS
    vtkNetDmfReader.cxx
    )

  ADD_PARAVIEW_PLUGIN(NetDMFReaderPlugin "1.0"
    SERVER_MANAGER_XML vtkNetDmfReader.xml 
    SERVER_MANAGER_SOURCES vtkNetDmfReader.cxx
    GUI_RESOURCE_FILES vtkNetDmfReaderGUI.xml)
  
  TARGET_LINK_LIBRARIES(
    NetDMFReaderPlugin
    NetDMF
    )

ENDIF(NetDMF_FOUND)


# create a plugin with a custom view that shows up in ParaView's multi-view
# manager.  this plugin also contains a custom display panel

SET(UI_FORMS
  NetDmfDisplayPanel.ui
)

# moc the Qt based .h files
QT4_WRAP_CPP(MOC_SRCS NetDmfView.h NetDmfDisplay.h GlyphRepresentation.h
  )

QT4_WRAP_UI(UI_BUILT_SOURCES
  ${UI_FORMS}
)

# invoke macro to create sources for our custom view and display panel
ADD_PARAVIEW_VIEW_MODULE(
                         # returns the interfaces defined (pass in
                         # GUI_INTERFACES parameter)
                         IFACES  
                         # returns a list of source files for this interface
                         IFACE_SRCS 
                         # give the view type 
                         # With MyView.h implementing a
                         # pqGenericViewModule and MyView being the XML name
                         # for the view on the server side
                         VIEW_TYPE NetDmfView 
                         # the XML group of the view in the server manager xml
                         VIEW_XML_GROUP views
                         # the XML name of the display for this view
                         DISPLAY_XML GlyphRepresentation 
                         # the name of the display panel for this display
                         # With MyDisplay.h implementing pqDisplayPanel
                         DISPLAY_PANEL NetDmfDisplay
                         DISPLAY_TYPE GlyphRepresentation
                         )

# Create a plugin with the new view and its options.
ADD_PARAVIEW_PLUGIN(NetDmfView "1.0"
  SERVER_MANAGER_XML NetDmfViewSM.xml
  SERVER_MANAGER_SOURCES  vtkSMGlyphRepresentationProxy.cxx 
  GUI_INTERFACES ${IFACES} ${OPTIONS_IFACE}
  GUI_SOURCES NetDmfView.cxx NetDmfDisplay.cxx GlyphRepresentation.cxx
  ${MOC_SRCS} ${UI_BUILT_SOURCES} ${IFACE_SRCS} ${OPTIONS_IFACE_SRCS}
  )
