PROJECT(KWCommonXML)

INCLUDE_REGULAR_EXPRESSION("^(vtk|kw|icons).*$")

INCLUDE_DIRECTORIES(
  ${KWCommonXML_SOURCE_DIR}
)

SET(KWCommonXML_SRCS
  vtkXMLCameraReader.cxx
  vtkXMLCameraWriter.cxx
  vtkXMLColorTransferFunctionReader.cxx
  vtkXMLColorTransferFunctionWriter.cxx
  vtkXMLObjectReader.cxx
  vtkXMLObjectWriter.cxx
  vtkXMLPiecewiseFunctionReader.cxx
  vtkXMLPiecewiseFunctionWriter.cxx
  vtkXMLPropertyReader.cxx
  vtkXMLPropertyWriter.cxx
  vtkXMLScalarsToColorsReader.cxx
  vtkXMLScalarsToColorsWriter.cxx
  vtkXMLTextPropertyReader.cxx
  vtkXMLTextPropertyWriter.cxx
  vtkXMLUtilities.cxx
  vtkXMLVolumePropertyReader.cxx
  vtkXMLVolumePropertyWriter.cxx
)

SET_SOURCE_FILES_PROPERTIES(
  vtkXMLObjectReader
  vtkXMLObjectWriter
  ABSTRACT
)

VTK_WRAP_TCL(vtkKWCommonXMLTCL 
  KWCommonXMLTCL_SRCS ${KWCommonXML_SRCS}
)

ADD_LIBRARY(vtkKWCommonXMLTCL 
  ${KWCommonXMLTCL_SRCS} ${KWCommonXML_SRCS}
)

TARGET_LINK_LIBRARIES(vtkKWCommonXMLTCL
  vtkKWCommonTCL
)

INSTALL_TARGETS(/lib vtkKWCommonXMLTCL)
