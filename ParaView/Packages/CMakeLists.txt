CMAKE_MINIMUM_REQUIRED(VERSION 1.4)

SET( AltIO_SRCS
  vtkBase64InputStream.cxx
  vtkBase64OutputStream.cxx
  vtkDataCompressor.cxx
  vtkInputStream.cxx
  vtkOutputStream.cxx
  vtkTableExtentTranslator.cxx
  vtkZLibDataCompressor.cxx

  vtkXMLDataElement.cxx
  vtkXMLDataParser.cxx
  vtkXMLReader.cxx
  vtkXMLWriter.cxx
  vtkXMLDataReader.cxx
  vtkXMLImageDataWriter.cxx
  vtkXMLImageDataReader.cxx
  vtkXMLStructuredDataWriter.cxx
  vtkXMLRectilinearGridWriter.cxx
  vtkXMLStructuredGridWriter.cxx
  vtkXMLStructuredDataReader.cxx
  vtkXMLRectilinearGridReader.cxx
  vtkXMLStructuredGridReader.cxx
  vtkXMLPStructuredDataWriter.cxx
  vtkXMLPStructuredDataReader.cxx
  vtkXMLPImageDataWriter.cxx
  vtkXMLPImageDataReader.cxx
  vtkXMLPRectilinearGridWriter.cxx
  vtkXMLPRectilinearGridReader.cxx
  vtkXMLPStructuredGridWriter.cxx
  vtkXMLPStructuredGridReader.cxx
  vtkXMLUnstructuredGridReader.cxx
  vtkXMLUnstructuredGridWriter.cxx
  vtkXMLUnstructuredDataWriter.cxx
  vtkXMLUnstructuredDataReader.cxx
  vtkXMLPolyDataWriter.cxx
  vtkXMLPolyDataReader.cxx
  vtkXMLPDataWriter.cxx
  vtkXMLPDataReader.cxx
  vtkXMLPUnstructuredDataWriter.cxx
  vtkXMLPUnstructuredDataReader.cxx
  vtkXMLPUnstructuredGridWriter.cxx
  vtkXMLPUnstructuredGridReader.cxx
  vtkXMLPPolyDataWriter.cxx
  vtkXMLPPolyDataReader.cxx

  vtkDataArraySelection.cxx
  vtkXMLDataSetWriter.cxx
  vtkXMLFileReadTester.cxx
)

SET_SOURCE_FILES_PROPERTIES(
  vtkDataCompressor.cxx

  vtkXMLWriter.cxx
  vtkXMLReader.cxx
  vtkXMLDataReader.cxx
  vtkXMLStructuredDataWriter.cxx
  vtkXMLStructuredDataReader.cxx
  vtkXMLPStructuredDataWriter.cxx
  vtkXMLPStructuredDataReader.cxx
  vtkXMLUnstructuredDataWriter.cxx
  vtkXMLUnstructuredDataReader.cxx

  vtkXMLPDataWriter.cxx
  vtkXMLPDataReader.cxx
  vtkXMLPUnstructuredDataWriter.cxx
  vtkXMLPUnstructuredDataReader.cxx

  ABSTRACT
)

#SET_SOURCE_FILES_PROPERTIES(
#  files here...
#  WRAP_EXCLUDE
#)

# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR(vtkAltIOInstantiator AltIOInstantiator_SRCS
                      ${AltIO_SRCS}
                      EXPORT_MACRO VTK_EXPORT
                      HEADER_LOCATION ${VTK_BINARY_DIR})
ADD_LIBRARY(vtkAltIO SHARED ${AltIO_SRCS} ${AltIOInstantiator_SRCS})

IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL(vtkAltIOTCL AltIOTCL_SRCS ${AltIO_SRCS})
  ADD_LIBRARY(vtkAltIOTCL SHARED ${AltIOTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkAltIOTCL
                         vtkAltIO
                         vtkIOTCL
                         vtkFilteringTCL 
                         vtkCommonTCL
                         ${TCL_LIBRARY})
ENDIF (VTK_WRAP_TCL)

TARGET_LINK_LIBRARIES (vtkAltIO
vtkIO
)

SUBDIRS(Testing)