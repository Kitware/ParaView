ADD_EXECUTABLE(vtkWrapClientServer vtkWrapClientServer.c vtkParse.tab.c)
INSTALL_TARGETS(/lib/paraview-${PARAVIEW_VERSION} vtkWrapClientServer)
