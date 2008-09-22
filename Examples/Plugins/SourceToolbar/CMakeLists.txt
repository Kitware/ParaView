
# We need to wrap for Qt stuff such as signals/slots etc. to work correctly.
QT4_WRAP_CPP(MOC_SRCS SourceToolbarActions.h)

# This is a macro for adding QActionGroup subclasses automatically as toolbars.
ADD_PARAVIEW_ACTION_GROUP(IFACES IFACE_SRCS 
                          CLASS_NAME SourceToolbarActions
                          GROUP_NAME "ToolBar/SourceToolbar")

# Now crete a plugin for the toolbar. Here we pass IFACES and IFACE_SRCS
# which are filled up by the above macro with relevant entries
ADD_PARAVIEW_PLUGIN(SourceToolbar "1.0"
                    GUI_INTERFACES ${IFACES}
                    SOURCES ${MOC_SRCS} ${IFACE_SRCS} 
                    SourceToolbarActions.cxx)

