
QT4_WRAP_CPP(MOC_SRCS MyView.h MyDisplay.h)

ADD_PARAVIEW_VIEW_MODULE(IFACES IFACE_SRCS VIEW_TYPE MyView 
                         VIEW_XML_GROUP views
                         DISPLAY_XML MyDisplay 
                         DISPLAY_PANEL MyDisplay)

ADD_PARAVIEW_PLUGIN(GUIMyView "1.0" GUI_INTERFACES ${IFACES} 
                    SOURCES MyView.cxx MyDisplay.cxx ${MOC_SRCS} ${IFACE_SRCS})

ADD_PARAVIEW_PLUGIN(SMMyView "1.0" SERVER_MANAGER_XML MyViewSM.xml)

