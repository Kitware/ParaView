PROJECT(WidgetsXML)

INCLUDE_REGULAR_EXPRESSION("^(vtk|kw|icons).*$")

INCLUDE_DIRECTORIES(${KWCommonXML_SOURCE_DIR})

SET(WidgetsXML_SRCS
  vtkXMLCornerAnnotationReader.cxx
  vtkXMLCornerAnnotationWriter.cxx
  vtkXMLKWRenderWidgetReader.cxx
  vtkXMLKWRenderWidgetWriter.cxx
  vtkXMLKWWindowReader.cxx
  vtkXMLKWWindowWriter.cxx
)

VTK_WRAP_TCL(vtkKWWidgetsXMLTCL 
  WidgetsXMLTCL_SRCS ${WidgetsXML_SRCS}
)

ADD_LIBRARY(vtkKWWidgetsXMLTCL 
  ${WidgetsXMLTCL_SRCS} ${WidgetsXML_SRCS}
)

TARGET_LINK_LIBRARIES(vtkKWWidgetsXMLTCL
  vtkKWCommonTCL
  vtkKWCommonXMLTCL
  vtkKWWidgetsTCL
)

INSTALL_TARGETS(/lib vtkKWWidgetsXMLTCL)
