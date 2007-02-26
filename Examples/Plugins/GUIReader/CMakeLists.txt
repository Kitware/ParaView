
# create a plugin that adds a reader to the ParaView GUI
# it is added in the file dialog when doing opens/saves.

# The qrc file is processed by Qt's resource compiler (rcc)
# the qrc file must have a resource prefix of "/ParaViewResources"
# and ParaView will read anything contained under that prefix
# the pqReader.xml file contains xml defining readers with their 
# file extensions and descriptions.

ADD_PARAVIEW_PLUGIN(GUIMyPNGReader "1.0" GUI_RESOURCES pqReader.qrc)

