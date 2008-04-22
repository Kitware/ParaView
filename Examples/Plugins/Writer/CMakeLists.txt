
# create a plugin that adds a reader to the ParaView GUI
# it is added in the file dialog when doing opens/saves.

IF (PARAVIEW_BUILD_QT_GUI)
  ADD_PARAVIEW_PLUGIN(MyTiffWriter "1.0"
    GUI_RESOURCE_FILES GUITiff.xml
    SERVER_MANAGER_XML writers.xml
    )
ELSE (PARAVIEW_BUILD_QT_GUI)
  ADD_PARAVIEW_PLUGIN(MyTiffWriter "1.0"
    SERVER_MANAGER_XML writers.xml
    )
ENDIF (PARAVIEW_BUILD_QT_GUI)

