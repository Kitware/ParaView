# vtkParse.tab.c has #line markers that break #include with double quotes.
INCLUDE_DIRECTORIES(${ParaView_SOURCE_DIR}/VTK/Wrapping)
ADD_EXECUTABLE(vtkWrapClientServer vtkWrapClientServer.c
               ${ParaView_SOURCE_DIR}/VTK/Wrapping/vtkParse.tab.c)
INSTALL_TARGETS(/lib/paraview-${PARAVIEW_VERSION} vtkWrapClientServer)
