set(Module_SRCS
  vtkXMLPDataWriter.cxx
  vtkXMLPolyDataWriter.cxx
  vtkXMLPPolyDataWriter.cxx
  vtkXMLPUnstructuredDataWriter.cxx
  vtkXMLUnstructuredDataWriter.cxx
  vtkXMLWriter.cxx
  )

set_source_files_properties(
  vtkXMLPDataWriter
  vtkXMLPUnstructuredDataWriter
  vtkXMLUnstructuredDataWriter
  vtkXMLWriter
  ABSTRACT
  )

vtk_module_library(vtkIOXML ${Module_SRCS})
