# Create an auto-start plugin. Auto start plugins provide callbacks that get
# called when the plugin is loaded and when the application shutsdown.

QT4_WRAP_CPP(MOC_SRCS pqMyApplicationStarter.h)

ADD_PARAVIEW_AUTO_START(IFACES IFACE_SRCS CLASS_NAME pqMyApplicationStarter
                        STARTUP onStartup
                        SHUTDOWN onShutdown)

# create a plugin for this starter
ADD_PARAVIEW_PLUGIN(Autostart "1.0" 
                    GUI_INTERFACES ${IFACES} 
                    SOURCES pqMyApplicationStarter.cxx ${MOC_SRCS} ${RCS_SRCS} ${IFACE_SRCS})

