
# create a plugin that adds a reader to the ParaView GUI
# it is added in the file dialog when doing opens/saves.

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

IF(NOT ParaView_BINARY_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF(NOT ParaView_BINARY_DIR)

IF(PARAVIEW_BUILD_QT_GUI)
  # The pqReader.xml file contains xml defining readers with their file
  # extensions and descriptions.  The Server Manager XML file defines the
  # properties of the reader.  This example uses the already existing PNG
  # reader but gives it another identity in the Server Manager.  Adding your
  # own reader to ParaView would mean the SourceProxy class points to your
  # class.
  ADD_PARAVIEW_PLUGIN(MyPNGReader "1.0"
    SERVER_MANAGER_XML readers.xml
    GUI_RESOURCE_FILES pqReader.xml
    )
ENDIF(PARAVIEW_BUILD_QT_GUI)
