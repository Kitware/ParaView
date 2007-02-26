
# create a plugin that implements an object panel for a source proxy

QT4_WRAP_CPP(MOC_SRCS pqConePanel.h)

# we implement a pqConePanel.h for the ConeSource
ADD_PARAVIEW_OBJECT_PANEL(IFACES IFACE_SRCS CLASS_NAME pqConePanel 
                          XML_NAME ConeSource XML_GROUP sources)

# create a plugin for this panel
ADD_PARAVIEW_PLUGIN(GUIConePanel "1.0" 
                    GUI_INTERFACES ${IFACES} 
                    SOURCES pqConePanel.cxx ${MOC_SRCS} ${IFACE_SRCS})

