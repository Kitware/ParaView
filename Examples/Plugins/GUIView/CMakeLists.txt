
# create a plugin with a custom view that shows up in ParaView's multi-view
# manager.  this plugin also contains a custom display panel

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

IF(NOT ParaView_BINARY_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF(NOT ParaView_BINARY_DIR)

IF(PARAVIEW_BUILD_QT_GUI)
  # moc the Qt based .h files
  QT4_WRAP_CPP(MOC_SRCS MyView.h MyDisplay.h MyViewActiveOptions.h MyViewOptions.h)

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
                           VIEW_TYPE MyView
                           # the XML group of the view in the server manager xml
                           VIEW_XML_GROUP views
                           # the XML name of the display for this view
                           DISPLAY_XML MyDisplay
                           # the name of the display panel for this display
                           # With MyDisplay.h implementing pqDisplayPanel
                           DISPLAY_PANEL MyDisplay)


  ADD_PARAVIEW_VIEW_OPTIONS(OPTIONS_IFACE OPTIONS_IFACE_SRCS
                            VIEW_TYPE MyView ACTIVE_VIEW_OPTIONS MyViewActiveOptions)

  # Create a plugin with the new view and its options.
  ADD_PARAVIEW_PLUGIN(MyView "1.0"
    SERVER_MANAGER_XML MyViewSM.xml
    GUI_INTERFACES ${IFACES} ${OPTIONS_IFACE}
    GUI_SOURCES MyView.cxx MyDisplay.cxx MyViewActiveOptions.cxx MyViewOptions.cxx
    ${MOC_SRCS} ${IFACE_SRCS} ${OPTIONS_IFACE_SRCS}
    )
ENDIF(PARAVIEW_BUILD_QT_GUI)
