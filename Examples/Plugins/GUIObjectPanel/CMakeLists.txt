
QT4_WRAP_CPP(MOC_SRCS pqConePanel.h)

ADD_PARAVIEW_OBJECT_PANEL(IFACES IFACE_SRCS pqConePanel ConeSource sources)

ADD_PARAVIEW_PLUGIN(GUIConePanel "1.0" 
                    GUI_INTERFACES ${IFACES} 
                    GUI_RESOURCES pqConePanel.qrc
                    SOURCES pqConePanel.cxx ${RCS_SRCS} ${MOC_SRCS} ${IFACE_SRCS})

