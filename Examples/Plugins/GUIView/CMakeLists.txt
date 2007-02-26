
# create a plugin with a custom view that shows up in ParaView's multi-view
# manager.  this plugin also contains a custom display panel

# moc the Qt based .h files
QT4_WRAP_CPP(MOC_SRCS MyView.h MyDisplay.h)

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

# create a GUI side plugin with the GUI side code
ADD_PARAVIEW_PLUGIN(GUIMyView "1.0" GUI_INTERFACES ${IFACES} 
                    SOURCES MyView.cxx MyDisplay.cxx ${MOC_SRCS} ${IFACE_SRCS})

# create a server side plugin with the server side code
ADD_PARAVIEW_PLUGIN(SMMyView "1.0" SERVER_MANAGER_XML MyViewSM.xml)

# one could combine the two plugins into one if desired

