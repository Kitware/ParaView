
# create a plugin that implements an object panel for a source proxy

QT4_WRAP_CPP(MOC_SRCS MyToolBarActions.h)

# we implement a pqConePanel.h for the ConeSource
ADD_PARAVIEW_ACTION_GROUP(IFACES IFACE_SRCS CLASS_NAME MyToolBarActions 
                          GROUP_NAME "ToolBar/MyActions")

# create a plugin for this panel
ADD_PARAVIEW_PLUGIN(MyToolBar "1.0" 
                    GUI_INTERFACES ${IFACES} 
                    SOURCES MyToolBarActions.cxx ${MOC_SRCS} ${IFACE_SRCS})


