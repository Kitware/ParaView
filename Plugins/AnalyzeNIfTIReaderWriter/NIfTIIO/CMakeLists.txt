set(classes
  vtkAnalyzeReader
  vtkAnalyzeWriter
  vtkNIfTIReader
  vtkNIfTIWriter)

set(private_classes
  ThirdParty/vtknifti1_io
  ThirdParty/vtkznzlib)

set(private_headers
  ThirdParty/vtkznzlib.h)

vtk_module_add_module(AnalyzeNIfTIIO::NIfTIIO
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes}
  PRIVATE_HEADERS ${private_headers})

paraview_add_server_manager_xmls(
  XMLS  AnalyzeNIfTISM.xml)
